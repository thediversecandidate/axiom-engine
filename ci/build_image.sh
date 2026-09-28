#!/usr/bin/env bash
# Builds (or reuses) the CI image, pushes it to GHCR and writes its digest to $GITHUB_OUTPUT.
set -euo pipefail
tag=$(cat ci/Dockerfile ci/packages.txt scripts/requirements.txt | sha256sum | cut -c1-16)
image="ghcr.io/${GITHUB_REPOSITORY_OWNER,,}/axiom-ci"
echo "$GH_TOKEN" | docker login ghcr.io -u "$GITHUB_ACTOR" --password-stdin
if docker pull "$image:$tag" > /dev/null 2>&1; then
  echo "reusing $image:$tag"
else
  docker build --progress=plain -t "$image:$tag" -f ci/Dockerfile .
  docker push "$image:$tag"
fi
ref=$(docker inspect --format '{{index .RepoDigests 0}}' "$image:$tag")
echo "ref=$ref" >> "$GITHUB_OUTPUT"
docker run --rm "$image:$tag" cat /opt/axiom-ci/packages.lock > packages.lock
echo "CI image: $ref"
docker pull -q ubuntu:26.04 > /dev/null
echo "::notice title=base image digest::$(docker inspect --format '{{index .RepoDigests 0}}' ubuntu:26.04)"
echo "::notice title=toolchain::$(docker run --rm "$image:$tag" sh -c 'clang-21 --version | head -1; cmake --version | head -1; python3 --version' | tr '\n' ';')"
