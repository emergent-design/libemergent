VERSION 0.8

FROM ubuntu:22.04
ENV DEBIAN_FRONTEND noninteractive
ENV DEBCONF_NONINTERACTIVE_SEEN true
WORKDIR /code

COPY --if-exists proxy.conf /etc/apt/apt.conf.d/30-proxy
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates curl build-essential cmake clang fakeroot chrpath dh-exec

code:
	COPY --dir include packages src CMakeLists.txt .

check:
	ARG PREMAKE=5.0.0-alpha16

	FROM +code

	RUN apt-get update && apt-get install -y --no-install-recommends libfreeimage-dev

	RUN cmake -B build \
		&& make -j8 -C build \
		&& make -C build test ARGS=--output-on-failure

	# RUN curl -sL -o premake.deb https://github.com/emergent-design/premake-pkg/releases/download/v$PREMAKE/premake_$PREMAKE-0ubuntu1_amd64.deb \
	# 	&& dpkg -i premake.deb
	# RUN premake5 gmake && make -j$(nproc)

package:
	FROM +code
	RUN cd packages && dpkg-buildpackage -b -uc -us
	SAVE ARTIFACT --keep-ts libemergent-dev_*.deb AS LOCAL build/
	# SAVE ARTIFACT --keep-ts libemergent-dev_*.deb libemergent-dev.deb

emergent-all:
	BUILD +package
