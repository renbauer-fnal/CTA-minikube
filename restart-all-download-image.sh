docker_auth="$(cat ~/.docker/config.json | grep -n2 "auth" | grep "imageregistry.fnal.gov")"
if [ -z "$docker_auth" ]; then
	echo "This script requires your docker to be logged in to imageregistry.fnal.gov, which it does not appear to be based on your config."
	echo "Please run \`docker login imageregistry.fnal.gov\` and provide your credentials"
	exit 1
fi
minikube stop;
minikube delete;
sudo systemctl stop docker;
sudo firewalld;
sudo systemctl start docker;
minikube start --disk-size=28g --disable-driver-mounts;
eval $(minikube -p minikube docker-env)
kubectl delete namespaces cta;
pushd ~/CTA-minikube/CTA/continuousintegration/buildtree_runner/;
  # ./prepareImage.sh;
  docker pull imageregistry.fnal.gov/cta-eval/buildtree-runner:latest
  docker tag imageregistry.fnal.gov/cta-eval/buildtree-runner:latest buildtree-runner:latest
  sudo ./recreate_buildtree_running_environment.sh;
popd;
pushd ~/CTA-minikube/nfs/;
  ./bootstrap-nfs.sh;
popd;
pushd ~/CTA-minikube/CTA/continuousintegration/orchestration/;
  ./create_instance.sh -n cta -b ~ -B CTA-build -D -O -d database.yaml;
popd;
