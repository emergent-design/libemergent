VERSION 0.6

FROM ubuntu:18.04
ENV DEBIAN_FRONTEND noninteractive
ENV DEBCONF_NONINTERACTIVE_SEEN true
WORKDIR /code

COPY --if-exists proxy.conf /etc/apt/apt.conf.d/30-proxy
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates curl build-essential clang fakeroot chrpath dh-exec

code:
	COPY --dir include packages src premake5.lua .

check:
	ARG PREMAKE=5.0.0-alpha16

	FROM +code
	RUN curl -L -o premake.deb https://github.com/emergent-design/premake-pkg/releases/download/v$PREMAKE/premake_$PREMAKE-0ubuntu1_amd64.deb \
		&& dpkg -i premake.deb
	RUN apt-get update && apt-get install -y --no-install-recommends libfreeimage-dev
	RUN premake5 gmake && make -j$(nproc)

package:
	FROM +code
	RUN cd packages && dpkg-buildpackage -b -uc -us
	SAVE ARTIFACT libemergent-dev_*.deb libemergent-dev.deb
	SAVE ARTIFACT libemergent-dev_*.deb AS LOCAL build/

emergent-all:
	BUILD +package

