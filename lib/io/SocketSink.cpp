/* <x0/SocketSink.cpp>
 *
 * This file is part of the x0 web server project and is released under LGPL-3.
 * http://www.xzero.ws/
 *
 * (c) 2009-2010 Christian Parpart <trapni@gentoo.org>
 */

#include <x0/io/SocketSink.h>
#include <x0/io/Source.h>
#include <x0/io/FileSource.h>
#include <x0/io/BufferSource.h>
#include <x0/io/FilterSource.h>
#include <x0/io/CompositeSource.h>
#include <x0/Socket.h>
#include <x0/sysconfig.h>

#if defined(HAVE_SYS_SENDFILE_H)
#	include <sys/sendfile.h>
#endif

namespace x0 {

SocketSink::SocketSink(Socket *s) :
	socket_(s),
	buf_(),
	offset_(0),
	result_(0)
{
}

ssize_t SocketSink::pump(Source& src)
{
	// call pump-handler
	src.accept(*this);

	return result_;
}

ssize_t SocketSink::genericPump(Source& src)
{
	if (buf_.empty())
		src.pull(buf_);

	std::size_t remaining = buf_.size() - offset_;
	if (!remaining)
		return 0;

	ssize_t nwritten = socket_->write(buf_.ref(offset_));

	if (nwritten != -1)
	{
		if (static_cast<std::size_t>(nwritten) == remaining)
		{
			buf_.clear();
			offset_ = 0;
		}
		else
			offset_ += nwritten;
	}

	return nwritten;
}

void SocketSink::visit(SystemSource& v)
{
	result_ = genericPump(v);
}

void SocketSink::visit(FileSource& v)
{
	if (!offset_)
		offset_ = v.offset(); // initialize with the starting-offset of interest

	if (std::size_t remaining = v.count() - offset_) // how many bytes are still to process
		result_ = socket_->write(v.handle(), &offset_, remaining);
	else
		result_ = 0;
}

void SocketSink::visit(BufferSource& v)
{
	result_ = genericPump(v);
}

void SocketSink::visit(FilterSource& v)
{
	result_ = genericPump(v);
}

void SocketSink::visit(CompositeSource& v)
{
	result_ = 0;

	while (!v.empty())
	{
		v.front()->accept(*this);
		if (result_ != 0)
			return;

		v.pop_front();
	}
}

void SocketSink::accept(SinkVisitor& v)
{
	v.visit(*this);
}

ssize_t SocketSink::write(const void *buffer, size_t size)
{
	return socket_->write(buffer, size);
}

} // namespace x0
