#FROM ghcr.io/kamailio/kamailio:5.7.2-focal
FROM ghcr.io/kamailio/kamailio:5.7.4-focal
#COPY kamailio /usr/sbin/
COPY modules/group.so /usr/lib/x86_64-linux-gnu/kamailio/modules/

#RUN apt update -y
##RUN apt upgrade -y
##RUN apt install -y libev-dev libc6
#RUN apt install -y libev-dev gawk bison flex gcc make wget tar
#RUN wget -c https://ftp.gnu.org/gnu/glibc/glibc-2.34.tar.gz
#RUN tar -zxvf glibc-2.34.tar.gz && cd glibc-2.34
#RUN mkdir glibc-build && cd glibc-build
#CMD ["../configure --prefix=/lib/x86_64-linux-gnu/"]
#CMD ["make"]
#CMD ["make install"]
#
#COPY libc.so.6 /lib/x86_64-linux-gnu/

CMD ["kamailio -f /etc/kamailio/kamailio.cfg -DD -E -m 800 -M 64"]
