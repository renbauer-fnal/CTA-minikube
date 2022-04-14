This directory includes directories and files that must be placed into ~/.minikube/files/ in order to mount files to the minikube container, so they can be passed to pods.

**NOTE: ~/.minikube/files/cta_rpms must also include the CTA RPMS**

These must be built from source and added to the ~/.minikube/files/cta_rpms dir, EXCEPT cta-migration-tools, this is okay to omit if there are issues with oracle dependencies.
