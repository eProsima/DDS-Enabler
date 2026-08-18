# DDS ENABLER TEST DOCKER

In order to build this docker image, use command in current directory:

```sh
docker build --rm -t ddsenabler_test:some_tag --build-arg "fastcdr_branch=v2.3.6" --build-arg "fastdds_branch=v3.6.2" --build-arg "devutils_branch=v1.5.3" --build-arg "ddspipe_branch=v1.5.2" --build-arg "ddsenabler_branch=v1.2.1" .
```
