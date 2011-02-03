/* <x0/FileSource.cpp>
 *
 * This file is part of the x0 web server project and is released under LGPL-3.
 * http://www.xzero.ws/
 *
 * (c) 2009-2010 Christian Parpart <trapni@gentoo.org>
 */

#include <x0/io/FileSource.h>
#include <x0/io/SourceVisitor.h>
#include <x0/io/BufferSink.h>
#include <x0/io/FileSink.h>
#include <x0/io/SocketSink.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace x0 {

FileSource::FileSource(const char *filename) :
	SystemSource(::open(filename, O_RDONLY), 0, 0),
	autoClose_(true)
{
	if (handle() > 0) {
		struct stat st;
		if (stat(filename, &st) == 0)
			count_ = st.st_size;
	}
}

/** initializes a file source.
 *
 * \param f the file to stream
 */
FileSource::FileSource(int fd, std::size_t offset, std::size_t size, bool autoClose) :
	SystemSource(fd, offset, size),
	autoClose_(autoClose)
{
}

FileSource::~FileSource()
{
	if (autoClose_)
		::close(handle());
}

void FileSource::accept(SourceVisitor& v)
{
	v.visit(*this);
}

ssize_t FileSource::sendto(Sink& output)
{
	output.accept(*this);
	return result_;
}

void FileSource::visit(BufferSink& v)
{
	char buf[8 * 4096];
	result_ = pread(handle(), buf, sizeof(buf), offset_);

	if (result_ > 0) {
		v.write(buf, result_);
		offset_ += result_;
		count_ -= result_;
	}
}

void FileSource::visit(FileSink& v)
{
#if 0
	// only works if target is a pipe.
	loff_t offset = offset_;
	result_ = ::splice(handle(), &offset, v.handle(), NULL, count_,
		SPLICE_F_MOVE | SPLICE_F_NONBLOCK | SPLICE_F_MORE);

	if (result_ > 0) {
		offset_ = offset;
		count_ -= result_;
	}
#endif
	char buf[8 * 1024];
	result_ = pread(handle(), buf, sizeof(buf), offset_);

	if (result_ <= 0)
		return;

	result_ = v.write(buf, result_);

	if (result_ <= 0)
		return;

	offset_ += result_;
	count_ -= result_;
}

void FileSource::visit(SocketSink& v)
{
	result_ = v.write(handle(), &offset_, count_);

	if (result_ > 0) {
		count_ -= result_;
	}
}

} // namespace x0
