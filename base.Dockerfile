######
# Base image of debian bookworm-slim and custom APT source: https://mirrors.aliyun.com/debian/
###
FROM debian:bookworm-slim
RUN rm -f /etc/apt/sources.list.d/debian.sources
RUN cat <<EOF > /etc/apt/sources.list
deb http://mirrors.aliyun.com/debian/ bookworm main non-free-firmware contrib
EOF
# Install ca-certificates before using https sources
RUN apt-get update -y && apt-get install -y --no-install-recommends ca-certificates

# Switch to https sources
RUN cat <<EOF > /etc/apt/sources.list
deb https://mirrors.aliyun.com/debian/ bookworm main non-free-firmware contrib
deb-src https://mirrors.aliyun.com/debian/ bookworm main non-free-firmware contrib
deb https://mirrors.aliyun.com/debian-security/ bookworm-security main non-free-firmware contrib
deb-src https://mirrors.aliyun.com/debian-security/ bookworm-security main non-free-firmware contrib
deb https://mirrors.aliyun.com/debian bookworm-updates main non-free-firmware contrib
deb-src https://mirrors.aliyun.com/debian bookworm-updates main non-free-firmware contrib
EOF

RUN apt-get update -y && \
    apt-get install -y locales && \
    localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8

CMD ["/bin/bash"]
