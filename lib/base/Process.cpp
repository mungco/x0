/* <x0/Process.cpp>
 *
 * This file is part of the x0 web server project and is released under LGPL-3.
 * http://www.xzero.ws/
 *
 * (c) 2009-2010 Christian Parpart <trapni@gentoo.org>
 */

#include <x0/Process.h>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

namespace x0 {

/** invokes \p cmd until its not early aborted with EINTR. */
#define EINTR_LOOP(rv, cmd) 				\
	do {									\
		rv = cmd;							\
	} while (rv == -1 && errno == EINTR)

Process::Process(struct ev_loop *loop) :
	loop_(loop),
	child_watcher_(loop_),
	input_(),
	output_(),
	error_(),
	pid_(-1),
	status_(0)
{
}

Process::Process(struct ev_loop *loop, const std::string& exe, const ArgumentList& args, const Environment& env, const std::string& workdir) :
	loop_(loop),
	input_(),
	output_(),
	error_(),
	pid_(-1),
	status_(0)
{
	start(exe, args, env, workdir);
}

Process::~Process()
{
	int rv;
	EINTR_LOOP(rv, ::waitpid(pid_, &status_, 0));
	//printf("~Process(): rv=%d, errno=%s\n", rv, strerror(errno));
}

int Process::start(const std::string& exe, const ArgumentList& args, const Environment& env, const std::string& workdir)
{
	//::fprintf(stderr, "proc[%d] start(exe=%s, args=[...], workdir=%s)\n", getpid(), exe.c_str(), workdir.c_str());
	switch (pid_ = fork())
	{
		case 0: // child
			setupChild(exe, args, env, workdir);
			break;
		case -1: // error
			return -1;
		default: // parent
			setupParent();
			break;
	}
	return 0;
}

/** sends SIGTERM (terminate signal) to the child Process.
 */
void Process::terminate()
{
	::kill(pid_, SIGTERM);
}

/** tests whether child Process has exited already.
 */
bool Process::expired()
{
	if (pid_ <= 0)
		return true;

	if (fetchStatus() == -1 && errno == ECHILD)
		return true;

	if (WIFEXITED(status_))
		return true;

	if (WIFSIGNALED(status_))
		return true;

	return false;
}

int Process::fetchStatus()
{
	int rv;

	do rv = ::waitpid(pid_, &status_, WNOHANG);
	while (rv == -1 && errno == EINTR);

	return rv;
}

void Process::setupParent()
{
	child_watcher_.set<Process, &Process::onChild>(this);
	child_watcher_.set(pid_);
	child_watcher_.start();

	// setup I/O
	input_.closeRemote();
	output_.closeRemote();
	error_.closeRemote();
}

void Process::onChild(ev::child&, int revents)
{
	printf("onChild(%d)\n", revents);
}

void Process::setupChild(const std::string& _exe, const ArgumentList& _args, const Environment& _env, const std::string& _workdir)
{
	// restore signal handler(s)
	::signal(SIGPIPE, SIG_DFL);

	// setup environment
	int k = 0;
	std::vector<char *> env(_env.size() + 1);

	for (Environment::const_iterator i = _env.cbegin(), e = _env.cend(); i != e; ++i)
	{
		char *buf = new char[i->first.size() + i->second.size() + 2];
		::memcpy(buf, i->first.c_str(), i->first.size());
		buf[i->first.size()] = '=';
		::memcpy(buf + i->first.size() + 1, i->second.c_str(), i->second.size() + 1);

		//::fprintf(stderr, "proc[%d]: setting env[%d]: %s\n", getpid(), k, buf);
		//::fflush(stderr);
		env[k++] = buf;
	}
	env[_env.size()] = 0;

	// setup args
	std::vector<char *> args(_args.size() + 2);
	args[0] = const_cast<char *>(_exe.c_str());
	//::fprintf(stderr, "args[%d] = %s\n", 0, args[0]);
	for (int i = 0, e = _args.size(); i != e; ++i)
	{
		args[i + 1] = const_cast<char *>(_args[i].c_str());
		//::fprintf(stderr, "args[%d] = %s\n", i + 1, args[i + 1]);
	}
	args[args.size() - 1] = 0;

	// chdir
	if (!_workdir.empty())
	{
		::chdir(_workdir.c_str());
	}

	// setup I/O
	int rv;
	EINTR_LOOP(rv, ::close(STDIN_FILENO));
	EINTR_LOOP(rv, ::close(STDOUT_FILENO));
	EINTR_LOOP(rv, ::close(STDERR_FILENO));

	EINTR_LOOP(rv, ::dup2(input_.remote(), STDIN_FILENO));
	EINTR_LOOP(rv, ::dup2(output_.remote(), STDOUT_FILENO));
	EINTR_LOOP(rv, ::dup2(error_.remote(), STDERR_FILENO));

//	input_.close();
//	output_.close();
//	error_.close();

	// finally execute
	::execve(args[0], &args[0], &env[0]);

	// OOPS
	::fprintf(stderr, "proc[%d]: execve(%s) error: %s\n", getpid(), args[0], strerror(errno));
	::fflush(stderr);
	::_exit(1);
}

} // namespace x0