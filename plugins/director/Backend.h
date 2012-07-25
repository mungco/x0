#pragma once

#include "HealthMonitor.h"

#include <x0/Counter.h>
#include <x0/Logging.h>
#include <x0/TimeSpan.h>
#include <x0/http/HttpRequest.h>

class Director;

/*!
 * \brief abstract base class for the actual proxying instances as used by \c Director.
 *
 * \see HttpBackend, FastCgiProxy
 */
class Backend
#ifndef NDEBUG
	: public x0::Logging
#endif
{
public:
	enum class Role {
		Active,
		Standby,
		Backup,
	};

protected:
	Director* director_; //!< director, this backend is registered to

	std::string name_; //!< common name of this backend, for example: "appserver05"
	size_t capacity_; //!< number of concurrent requests being processable at a time.
	x0::Counter load_; //!< number of active (busy) connections

	Role role_; //!< backend role (Active or Standby)
	bool enabled_; //!< whether or not this director is enabled (default) or disabled (for example for maintenance reasons)
	HealthMonitor healthMonitor_; //!< health check timer

	friend class Director;

public:
	Backend(Director* director, const std::string& name, size_t capacity = 1);
	virtual ~Backend();

	const std::string& name() const { return name_; }		//!< descriptive name of backend.
	Director* director() const { return director_; }		//!< pointer to the owning director.

	size_t capacity() const;								//!< number of requests this backend can handle in parallel.
	void setCapacity(size_t value);

	const x0::Counter& load() const { return load_; }		//!< number of currently being processed requests.

	// role
	Role role() const { return role_; }
	void setRole(Role value) { role_ = value; }

	// enable/disable state
	void enable() { enabled_ = true; }
	bool isEnabled() const { return enabled_; }
	void setEnabled(bool value) { enabled_ = value; }
	void disable() { enabled_ = false; }

	// health state
	HealthMonitor::State healthState() const { return healthMonitor_.state(); }
	HealthMonitor& healthMonitor() { return healthMonitor_; }

	virtual bool process(x0::HttpRequest* r) = 0;

	virtual size_t writeJSON(x0::Buffer& output) const;

	void release();

protected:
	void setState(HealthMonitor::State value);
};

/*! dummy proxy, just returning 503 (service unavailable).
 */
class NullProxy : public Backend {
public:
	NullProxy(Director* director, const std::string& name, size_t capacity);

	virtual bool process(x0::HttpRequest* r);
};

class FastCgiProxy : public Backend {
public:
	FastCgiProxy(Director* director, const std::string& name, size_t capacity, const std::string& url);
	~FastCgiProxy();

	virtual bool process(x0::HttpRequest* r);
	virtual size_t writeJSON(x0::Buffer& output) const;
};