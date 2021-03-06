include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

add_definitions(
	-Dlinux=1
	-Wall -Wno-deprecated
	-pthread
	-std=c++0x
	-fvisibility=hidden
	-DPACKAGE_NAME="${PACKAGE_NAME}"
	-DPACKAGE_VERSION="${PACKAGE_VERSION}"
	-DPACKAGE_HOMEPAGE_URL="${PACKAGE_HOMEPAGE_URL}"
	-DSYSCONFDIR="${SYSCONFDIR}"
	-DVERSION="${PACKAGE_VERSION}")

# ---------------------------------------------------------------------------

set(director_SRC
	DirectorPlugin.cpp
	HealthMonitor.cpp
	ApiRequest.cpp
	BackendManager.cpp
	BackendCluster.cpp
	Director.cpp
	Backend.cpp
	Scheduler.cpp
	HttpBackend.cpp
	HttpHealthMonitor.cpp
	FastCgiBackend.cpp
	FastCgiHealthMonitor.cpp
	RoadWarrior.cpp
	HaproxyApi.cpp
)

# XXX classful scheduling is work-in-progress
#	ClassfulScheduler.cpp
#	LeastLoadScheduler.cpp

x0_plugin(director "${director_SRC}")
