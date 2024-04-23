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

RUN mkdir -p /target/usr && rm -rf /usr/local && ln -sf /target/usr /usr/local
WORKDIR /usr/local/src/satdump
COPY . .

RUN cmake -B build \
          -DCMAKE_STAGING_PREFIX=/target/usr \
          -DCMAKE_INSTALL_PREFIX=/usr \
          -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_GUI=ON \
          -DPLUGIN_SCRIPTING=ON \
          -DBUILD_TOOLS=ON &&\
    cmake --build build --target install


ARG DEBIAN_IMAGE_TAG=bookworm
FROM debian:${DEBIAN_IMAGE_TAG} AS runner

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

COPY packages.runner /usr/local/src/
RUN apt -y update && \
    apt -y upgrade && \
    xargs -a /usr/local/src/packages.runner apt install -qy && \
    rm -rf /var/lib/apt/lists/*
COPY --from=builder /target /

