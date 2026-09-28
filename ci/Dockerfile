# Axiom CI image (CONVENTIONS §1, ADR-0002).
# Every apt package is pinned through the Ubuntu snapshot service: the whole archive is frozen at
# AXIOM_APT_SNAPSHOT, and the exact resolved versions are written to /opt/axiom-ci/packages.lock.
# Changing the snapshot, the base image or the package list requires an ADR.
FROM ubuntu:26.04

ARG AXIOM_APT_SNAPSHOT=20260927T000000Z
ENV DEBIAN_FRONTEND=noninteractive

COPY ci/packages.txt /opt/axiom-ci/packages.txt
RUN apt-get update --snapshot "${AXIOM_APT_SNAPSHOT}" \
 && xargs -a /opt/axiom-ci/packages.txt apt-get install -y --no-install-recommends --snapshot "${AXIOM_APT_SNAPSHOT}" \
 && dpkg-query -W -f='${Package} ${Version}\n' > /opt/axiom-ci/packages.lock \
 && echo "snapshot ${AXIOM_APT_SNAPSHOT}" >> /opt/axiom-ci/packages.lock \
 && rm -rf /var/lib/apt/lists/*

COPY scripts/requirements.txt /opt/axiom-ci/requirements.txt
RUN python3 -m venv /opt/venv \
 && /opt/venv/bin/pip install --no-cache-dir -r /opt/axiom-ci/requirements.txt \
 && /opt/venv/bin/pip freeze >> /opt/axiom-ci/packages.lock

ENV PATH="/opt/venv/bin:${PATH}"
