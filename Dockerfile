# BUILD
FROM gcc:11 as build
WORKDIR /_tmp

# Install
RUN apt update && \
    apt upgrade -y && \
    apt install -y \
      git \
      pip \
    && \
    pip install --upgrade pip && \
    pip install conan
    
ARG CMAKE_VERSION=3.25.0
RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.sh \
      -q -O /tmp/cmake-install.sh \
      && chmod u+x /tmp/cmake-install.sh \
      && mkdir /usr/bin/cmake \
      && /tmp/cmake-install.sh --skip-license --prefix=/usr/bin/cmake \
      && rm /tmp/cmake-install.sh
ENV PATH="/usr/bin/cmake/bin:${PATH}"
    
    
# Add sources
ADD ./App                       /src/App
ADD ./CMakeLists.txt            /src
ADD ./conanfile.txt             /src
ADD ./conan_profile_linux.txt   /src

# Build
WORKDIR /src/_build
RUN conan install .. --profile ../conan_profile_linux.txt --build=missing && \
    cmake .. -DCMAKE_MODULE_PATH="$PWD" && \
    cmake --build . --config Release
    
    
# Run
FROM ubuntu:latest

RUN groupadd -r app_user && useradd -r -g app_user app_user
USER app_user

WORKDIR /app

COPY --from=build /src/_build/bin/App .

ENTRYPOINT ["./App"]
