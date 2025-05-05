ARG DEBIAN_IMAGE_TAG=bookworm
FROM debian:${DEBIAN_IMAGE_TAG} AS builder

ARG DEBIAN_FRONTEND=noninteractive
ARG CMAKE_BUILD_PARALLEL_LEVEL
ENV TZ=Etc/UTC

WORKDIR /usr/local/src/
COPY packages.builder .
RUN apt -y update && \
    apt -y upgrade && \
    xargs -a packages.builder apt install --no-install-recommends -qy

WORKDIR /usr/local/src/satdump
COPY . .

RUN cmake -B build \
          -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_GUI=ON \
          -DPLUGIN_SCRIPTING=ON \
          -DBUILD_TOOLS=ON &&\
    cmake --build build --target package


ARG DEBIAN_IMAGE_TAG=bookworm
FROM debian:${DEBIAN_IMAGE_TAG} AS runner

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

COPY packages.runner /usr/local/src/
COPY --from=builder /usr/local/src/satdump/build/satdump_*.deb /usr/local/src/
RUN apt -y update && \
    apt -y upgrade && \
    xargs -a /usr/local/src/packages.runner apt install -qy && \
    apt install -qy /usr/local/src/satdump_*.deb && \
    rm -rf /var/lib/apt/lists/*

# Add a user, possibility to map it to a user on the host to get the same uid & gid on files
ARG HOST_UID=1000
ARG HOST_GID=1000
RUN groupadd -r -g ${HOST_GID} satdump && \
	useradd -r -u ${HOST_UID} \
            -g satdump \
            -d /srv \
            -s /bin/bash \
            -G audio,dialout,plugdev \
            -m \
            satdump
USER satdump
WORKDIR /srv
