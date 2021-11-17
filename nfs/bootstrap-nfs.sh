eval $(minikube -p minikube docker-env) || echo 'failed to set docker to minikube env'
kubectl delete namespace cta;
kubectl delete pv nfs;
kubectl create namespace cta;
for yaml in provisioner/local nfs-server-rc nfs-server-service; do
  kubectl apply -n cta -f ${yaml}.yaml;
done;
IP=""
while [[ $IP != *.*.*.* ]]; do
  sleep 1;
  IP=$(kubectl describe service nfs-server -n cta | grep Endpoints | head -1 | awk '{print $2}' | awk -F: '{print $1}';)
done;
cat nfs-pv.yaml | sed "s/SERVERIP/$IP/" > tmp-nfs-pv.yaml
for yaml in tmp-nfs-pv nfs-pvc; do
  kubectl apply -n cta -f ${yaml}.yaml;
done;
