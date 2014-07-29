FROM tianon/centos:5.8

#ADD http://pkgs.repoforge.org/rpmforge-release/rpmforge-release-0.5.3-1.el5.rf.x86_64.rpm /root/
#RUN rpm --import http://apt.sw.be/RPM-GPG-KEY.dag.txt
#RUN rpm -K rpmforge-release-0.5.3-1.el5.rf.*.rpp
#RUN rpm -i rpmforge-release-0.5.3-1.el5.rf.*.rpm

RUN yum update -y
RUN yum install -y make gcc44
RUN ln -s /usr/bin/gcc44 /usr/bin/gcc

ADD http://prdownloads.sourceforge.net/tcl/tcl8.5.15-src.tar.gz /root/
RUN cd /root && tar xf tcl8.5.15-src.tar.gz
RUN cd /root/tcl8.5.15/unix && ./configure --prefix=/opt/tcl && make && make install
ENV PATH /opt/tcl/bin:$PATH
ENV LD_LIBRARY_PATH /opt/tcl/lib
ENV LIBRARY_PATH /opt/tcl/lib

RUN yum install -y zlib-devel sqlite-devel
ADD https://www.python.org/ftp/python/2.7.8/Python-2.7.8.tgz /root/
RUN cd /root && tar xf /root/Python-2.7.8.tgz
RUN cd /root/Python-2.7.8 && ./configure --enable-shared --prefix=/opt/python && make && make install
ENV PATH /opt/python/bin:$PATH
ENV LD_LIBRARY_PATH /opt/python/lib
ENV LIBRARY_PATH /opt/python/lib

ADD . /root/libtclpy
RUN ln -s /opt/tcl/bin/tclsh8.5 /opt/tcl/bin/tclsh
RUN cd /root/libtclpy && make TCLCONFIG=/opt/tcl/lib/tclConfig.sh && make test
