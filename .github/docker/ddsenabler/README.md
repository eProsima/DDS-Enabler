# DDS ENABLER TEST DOCKER

In order to build this docker image, use command in current directory:

```sh
docker build --rm -t ddsenabler_test:some_tag --build-arg "fastcdr_branch=v2.3.5" --build-arg "fastdds_branch=v3.6.1" --build-arg "devutils_branch=v1.5.1" --build-arg "ddspipe_branch=v1.5.1" --build-arg "ddsenabler_branch=v1.2.0" .
```
