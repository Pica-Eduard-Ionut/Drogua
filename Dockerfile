# Build using: podman build -t localhost/drogon-alpine:latest .
# Alpine Linux build environment for the app

FROM alpine:3.24.2

ARG USER=drogon
ARG UID=1000
ARG GID=1000
ARG USER_HOME=/drogon

ENV TZ=UTC

# System timezone
RUN apk add --no-cache \
        tzdata \
    && ln -snf /usr/share/zoneinfo/${TZ} /etc/localtime \
    && echo "${TZ}" > /etc/timezone

# Build dependencies and Drogon dependencies
RUN apk add --no-cache \
        sudo \
        curl \
        wget \
        cmake \
        make \
        pkgconf \
        git \
        gcc \
        g++ \
        openssl \
        openssl-dev \
        jsoncpp-dev \
        util-linux-dev \
        zlib-dev \
        c-ares-dev \
        postgresql-dev \
        mariadb-dev \
        sqlite-dev \
        hiredis-dev \
        lua5.5 \
        lua5.5-dev

# Create non-root build user
RUN addgroup -S -g ${GID} ${USER} \
    && adduser -D -u ${UID} -G ${USER} -h ${USER_HOME} ${USER} \
    && mkdir -p /etc/sudoers.d \
    && echo "${USER} ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/${USER} \
    && chmod 0440 /etc/sudoers.d/${USER}

USER ${USER}
WORKDIR ${USER_HOME}

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8 \
    CC=gcc \
    CXX=g++ \
    AR=gcc-ar \
    RANLIB=gcc-ranlib \
    DROGON_INSTALLED_ROOT=${USER_HOME}/install

# Get Drogon source
RUN wget -O ${USER_HOME}/version.json \
        https://api.github.com/repos/an-tao/drogon/git/refs/heads/master \
    && git clone \
        https://github.com/an-tao/drogon \
        ${DROGON_INSTALLED_ROOT}

# Build and install Drogon
RUN cd ${DROGON_INSTALLED_ROOT} \
    && sed -i 's/bash/sh/' ./build.sh \
    && ./build.sh \
    && cmake --install build \
        --prefix ${DROGON_INSTALLED_ROOT}/local

ENV PATH="${DROGON_INSTALLED_ROOT}/local/bin:${PATH}"
ENV CMAKE_PREFIX_PATH="${DROGON_INSTALLED_ROOT}/local"
