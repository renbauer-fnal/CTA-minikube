# @project        The CERN Tape Archive (CTA)
# @copyright      Copyright(C) 2015-2021 CERN
# @license        This program is free software: you can redistribute it and/or modify
#                 it under the terms of the GNU General Public License as published by
#                 the Free Software Foundation, either version 3 of the License, or
#                 (at your option) any later version.
#
#                 This program is distributed in the hope that it will be useful,
#                 but WITHOUT ANY WARRANTY; without even the implied warranty of
#                 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#                 GNU General Public License for more details.
#
#                 You should have received a copy of the GNU General Public License
#                 along with this program.  If not, see <http://www.gnu.org/licenses/>.

# CTA generic image for system tests
# FROM must be the first command in Dockerfile!!

FROM buildtree-runner-stage1

# my environment variables
ENV ORCHESTRATIONDIR="continuousintegration/orchestration/pods" \
    LOCALORCHESTRATIONDIR="/opt/ci" \
    BASEDIR="continuousintegration/docker/ctafrontend/cc7" \
    CTAREPODIR="/tmp/repo"

# Update versionlock if needed
ADD ${BASEDIR}/etc/yum/pluginconf.d /etc/yum/pluginconf.d

# Install eos packages.
RUN yum install -y\
      eos-client \
      eos-server \
  && \
    yum clean all \
  && \
    rm -rf /var/cache/yum

# Docker image run setup
ADD ${BASEDIR}/run.sh /

ENTRYPOINT ["/run.sh"]
