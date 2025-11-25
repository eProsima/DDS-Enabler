# DDS ENABLER TEST DOCKER

In order to build this docker image, use command in current directory:

```sh
docker build --rm -t ddsenabler_test:some_tag --build-arg "fastcdr_branch=v2.3.4" --build-arg "fastdds_branch=v3.4.1" --build-arg "devutils_branch=v1.4.0" --build-arg "ddspipe_branch=v1.4.0" --build-arg "ddsenabler_branch=v1.1.0" .
```
