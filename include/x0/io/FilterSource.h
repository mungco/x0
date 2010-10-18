/* <FilterSource.h>
 *
 * This file is part of the x0 web server project and is released under LGPL-3.
 * http://www.xzero.ws/
 *
 * (c) 2009-2010 Christian Parpart <trapni@gentoo.org>
 */

#ifndef sw_x0_io_FilterSource_h
#define sw_x0_io_FilterSource_h 1

#include <x0/io/Source.h>
#include <x0/io/BufferSource.h>
#include <x0/io/Filter.h>
#include <x0/io/SourceVisitor.h>
#include <memory>

namespace x0 {

//! \addtogroup io
//@{

/** Filter source.
 */
class X0_API FilterSource :
	public Source
{
public:
	explicit FilterSource(Filter& Filter) :
		buffer_(), source_(std::make_shared<BufferSource>("")), filter_(Filter), force_(false) {}

	FilterSource(const SourcePtr& source, Filter& Filter, bool force) :
		buffer_(), source_(source), filter_(Filter), force_(force) {}

	virtual BufferRef pull(Buffer& output);
	virtual void accept(SourceVisitor& v);

protected:
	Buffer buffer_;
	SourcePtr source_;
	Filter& filter_;
	bool force_;
};

//@}

} // namespace x0

#endif