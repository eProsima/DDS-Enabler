FROM ubuntu:22.04
LABEL author=raul@eprosima.com

# Avoid interactuation with installation of some package that needs the locale.
ENV TZ=Europe/Madrid
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Avoids using interactions during building
ENV DEBIAN_FRONTEND=noninteractive

# Use a bash shell so it is possigle to run things like `source` (required for colcon builds)
SHELL ["/bin/bash", "-c"]

# Install apt dependencies
RUN apt-get update && apt-get install --yes --no-install-recommends \
    git \
    build-essential \
    cmake \
    python3-pip \
    wget \
    software-properties-common \
    zip \
    python3 \
    gradle \
    openjdk-8-jdk \
    libssl-dev \
    libasio-dev \
    libtinyxml2-dev \
    openssl \
    libyaml-cpp-dev

# Install required python modules
RUN pip3 install \
        colcon-common-extensions \
        colcon-mixin \
        lxml \
        vcstool \
        GitPython

WORKDIR /ddsenabler


ARG fastcdr_branch=master
ARG fastdds_branch=master
ARG devutils_branch=main
ARG ddspipe_branch=main
ARG ddsenabler_branch=main

RUN echo "fastcdr branch: <${fastcdr_branch}>" && \
    echo "fastdds branch: <${fastdds_branch}>" && \
    echo "devutils branch: <${devutils_branch}>" && \
    echo "ddspipe branch: <${ddspipe_branch}>"  && \
    echo "ddsenabler branch: <${ddsenabler_branch}>"

RUN mkdir src && \
    wget https://raw.githubusercontent.com/eProsima/FIWARE-DDS-Enabler/${ddsenabler_branch}/ddsenabler.repos && \
    vcs import src < ddsenabler.repos && \
    cd src/fastcdr && git checkout ${fastcdr_branch} && cd ../.. && \
    cd src/fastdds && git checkout ${fastdds_branch} && cd ../.. && \
    cd src/dev-utils && git checkout ${devutils_branch} && cd ../.. && \
    cd src/ddspipe && git checkout ${ddspipe_branch} && cd ../.. && \
    cd src/ddsenabler && git checkout ${ddsenabler_branch} && cd ../.. && \
    cp -r src/fastdds/examples/cpp/configuration src/. && \
    colcon build --packages-skip ddsenabler ddsenabler_docs --event-handlers console_direct+ --cmake-args -DLOG_INFO=ON -DSECURITY=ON && \
    colcon build --packages-select ddsenabler --event-handlers console_direct+ --cmake-args -DLOG_INFO=ON -DCOMPILE_EXAMPLES=ON

COPY run.bash /root/run.bash

RUN chmod +x /root/run.bash

ENTRYPOINT ["/root/run.bash" ]
