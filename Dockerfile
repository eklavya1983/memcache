FROM ubuntu:14.04
RUN apt-get update && apt-get install -yq software-properties-common
RUN add-apt-repository ppa:george-edison55/cmake-3.x
RUN add-apt-repository ppa:ubuntu-toolchain-r/test

RUN apt-get update && apt-get install -yq \
    autoconf-archive \
    bison \
    build-essential \
    cmake \
    curl \
    flex \
    gcc-4.9 \
    gdb \
    git \
    gperf \
    g++-4.9 \
    libcap-dev \
    libdouble-conversion-dev \
    libevent-dev \
    libgflags-dev \
    libgoogle-glog-dev \
    libjemalloc-dev \
    libkrb5-dev \
    libnuma-dev \
    libsasl2-dev \
    libsnappy-dev \
    libssl-dev \
    pkg-config \
    python \
    sudo \
    unzip \
    wget

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 60 --slave /usr/bin/g++ g++ /usr/bin/g++-4.9

RUN apt-get install -yq libboost-all-dev

# Install Java. From https://github.com/dockerfile/java/blob/master/oracle-java8/Dockerfile
RUN \
  echo oracle-java8-installer shared/accepted-oracle-license-v1-1 select true | debconf-set-selections && \
  add-apt-repository -y ppa:webupd8team/java && \
  apt-get update && \
  apt-get install -y oracle-java8-installer && \
  rm -rf /var/lib/apt/lists/* && \
  rm -rf /var/cache/oracle-jdk8-installer
# Define commonly used JAVA_HOME variable
ENV JAVA_HOME /usr/lib/jvm/java-8-oracle

RUN apt-add-repository universe
RUN apt-get update
RUN apt-get install -yq python-pip
RUN pip install future
RUN pip install futures
RUN pip install tabulate
