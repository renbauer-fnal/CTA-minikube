minikube stop;
minikube delete;
sudo systemctl stop docker;
sudo firewalld;
sudo systemctl start docker;
minikube start --disk-size=28g --disable-driver-mounts;
eval $(minikube -p minikube docker-env)
kubectl delete namespaces cta;
pushd ~/CTA-minikube/CTA/continuousintegration/buildtree_runner/;
  ./prepareImage.sh;
  sudo ./recreate_buildtree_running_environment.sh;
popd;
pushd ~/CTA-minikube/nfs/;
  ./bootstrap-nfs.sh;
popd;
pushd ~/CTA-minikube/CTA/continuousintegration/orchestration/;
  ./create_instance.sh -n cta -b ~ -B CTA-build -D -O -d database.yaml;
popd;
