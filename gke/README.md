# Running ThingsDB in Google Kubernetes Engine (GKE)

The goal of this blog is to show how ThingsDB can be deployed in GKE using three nodes for redundancy with automatic backups in Google Cloud Storage.

First an overview of all the kubernetes component we will use:

- [Service](#serive) - For connecting to ThingsDB.
- [ConfigMap](#configmap) - For service account credentials to Google Cloud Storage where we will store backups.
- [StatefulSet](#statefulset) - For deploying ThingsDB nodes.


## Service

This will expose the ThingsDB related ports to be used by other nodes. Not all ports are required although I don’t think it will do any harm. Important setting is `publishNotReadyAddresses` which must be set to `true`. If not, then the nodes will not publish themself before they are marked as ready and this is required as nodes need to synchronize data before they will be marked as ready.

```yaml
apiVersion: v1
kind: Service
metadata:
  labels:
    app: thingsdb
  name: thingsdb
spec:
  clusterIP: None
  publishNotReadyAddresses: true
  ports:
  - name: status
    port: 8080
  - name: client
    port: 9200
  - name: http
    port: 9210
  - name: node
    port: 9220
  selector:
    app: thingsdb
```

Save the above as `service.yaml` and apply the service. (or download the file [here](service.yaml))

```
kubectl apply -f service.yaml
```

## ConfigMap

The configMap is only required for creating backups in the Google Cloud Storage. If you just want to run ThingsDB in kubernetes without GKE, you may create your own backup solution and forget about
creating the config map. We store a service account key file in the config map.

To create a service account:

1. Open [Service accounts](https://console.cloud.google.com/iam-admin/serviceaccounts) on the Google Cloud Platform.
2. Click `+ CREATE SERVICE ACCOUNT` to create a new service account.
3. Enter a display name and service account ID, for example `thingsdb-sa`. (optionally enter a description)
4. On the next screen, *"Grant this service account access to project"*, select the role `Storage Object Creator` which should be sufficient for writing objects to Google Cloud Storage.
5. We do not require to *"Grant users access to this service account"*, leave the *user* and *admin* roles empty and press `DONE`.
6. At this point, the service account is created and by using the action `Create key` we can create our key file.
7. Choose `JSON` as the key type and you done.

Now open the JSON file and copy the context to [configmap.yaml](configmap.yaml) like in the example below:

```yaml
kind: ConfigMap
apiVersion: v1

metadata:
  name: ti-config
  namespace: default

data:
  service_account.json: |-
    {
        "type": "service_account",
        "project_id": "EXAMPLE PROJECT ID",
        "private_key_id": "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
        "private_key": "-----BEGIN PRIVATE KEY-----\n....\n-----END PRIVATE KEY-----\n",
        "client_email": "thingsdb-sa@dexample-project.iam.gserviceaccount.com",
        "client_id": "012345678901234567890",
        "auth_uri": "https://accounts.google.com/o/oauth2/auth",
        "token_uri": "https://oauth2.googleapis.com/token",
        "auth_provider_x509_cert_url": "https://www.googleapis.com/oauth2/v1/certs",
        "client_x509_cert_url": "https://www.googleapis.com/robot/v1/metadata/x509/thingsdb-sa%40example-project.iam.gserviceaccount.com"
      }

```

Apply the config map:

```
kubectl apply -f configmap.yaml
```

## StatefulSet

Now we only need a StatefulSet to deploy ThingsDB. In this example we deploy three ThingsDB nodes which enables enough redundancy for most scenarios.

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: thingsdb
  labels:
    app: thingsdb
spec:
  selector:
    matchLabels:
      app: thingsdb
  serviceName: thingsdb
  replicas: 3  # Three nodes for redundancy
  updateStrategy:
    type: RollingUpdate
  podManagementPolicy: Parallel
  template:
    metadata:
      labels:
        app: thingsdb
    spec:
      terminationGracePeriodSeconds: 90
      dnsConfig:
        searches:
        - thingsdb.default.svc.cluster.local
      containers:
      - name: thingsdb
        image: thingsdb/node:gcloud-v0.9.9  # Latest version at the time of writing
        imagePullPolicy: Always
        args: ["--deploy"]
        env:
        - name: THINGSDB_HTTP_STATUS_PORT
          value: "8080"
        - name: THINGSDB_HTTP_API_PORT
          value: "9210"
        - name: THINGSDB_STORAGE_PATH
          value: /mnt/thingsdb/
        - name: THINGSDB_NODE_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: THINGSDB_GCLOUD_KEY_FILE
          value: /mnt/config/service_account.json
        ports:
        - name: status
          containerPort: 8080
        - name: client
          containerPort: 9200
        - name: http
          containerPort: 9210
        - name: node
          containerPort: 9220
        volumeMounts:
        - name: data
          mountPath: /mnt/thingsdb
        - name: ti-config-volume
          mountPath: /mnt/config
        resources:
          requests:
            memory: 512Mi
        livenessProbe:
          httpGet:
            path: /healthy
            port: 8080
          periodSeconds: 10
          timeoutSeconds: 5
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 10
          timeoutSeconds: 2
      volumes:
        - name: ti-config-volume
          configMap:
            name: ti-config
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 10Gi
```


Apply the StatefulSet: (the file may be copied from above or can be downloaded [here](statefulset.yaml))

```
kubectl apply -f statefulset.yaml
```

## Configuring ThingsDB

We can now test if thingsdb is working. To do this we use `kubectl` port forwarding to create a connection to ThingsDB.

```bash
kubectl port-forward thingsdb-0 9210:9210
```

This will bind the local port `9210` to the HTTP port on the first ThingsDB node and allows for sending POST requests to query ThingsDB.

**Tip:** As an alternative, you might want bind to client port `9200` and use [ThingsDB GUI](http://github.com/thingsdb/ThingsGUI/releases/latest) instead of the CURL commands below.

```bash
curl --location --request POST 'http://localhost:9210/thingsdb' \
--header 'Content-Type: application/json' \
--user admin:pass \
--data-raw '{
    "type": "query",
    "code": "'Hello ThingsDB on GKE!';"
}'
```

If everything is working, you should see `Hello ThingsDB on GKE!` as a response.

Now lets look at the log of thingsdb node 1:

```
kubectl logs thingsdb-1
```

It will show you something similar to this:

```
Waiting for an invite from a node to join ThingsDB...

You can use the following query to add this node:

    new_node('thingsb-1', '10.0.0.11', 9220);
```

Copy the `new_node(...)` part and past it into a curl command, like this:

```bash
curl --location --request POST 'http://localhost:9210/thingsdb' \
--header 'Content-Type: application/json' \
--user admin:pass \
--data-raw '{
    "type": "query",
    "code": "new_node('thingsdb-1', '10.0.0.11', 9220);"
}'
```

Repeat the step above for node 2.

It might take a minute for the nodes to become ready. To view the status of the nodes the [nodes_info()](https://docs.thingsdb.net/v0/node-api/nodes_info/) function can be used.
Note that we use `/node` for this request since the nodes_info(..) function must be used from the node scope.

```bash
curl --location --request POST 'http://localhost:9210/node' \
--header 'Content-Type: application/json' \
--user admin:pass \
--data-raw '{
    "type": "query",
    "code": "nodes_info();"
}'
```

The output will be something similar to this:

```json
[
    {
        "node_id": 0,
        "syntax_version": "v0",
        "status": "READY",
        "zone": 0,
        "committed_event_id": 5,
        "stored_event_id": 4,
        "next_thing_id": 6,
        "node_name": "thingsdb-0",
        "port": 9220,
        "stream": null
    },
    {
        "node_id": 1,
        "syntax_version": "v0",
        "status": "READY",
        "zone": 0,
        "committed_event_id": 5,
        "stored_event_id": 4,
        "next_thing_id": 6,
        "node_name": "thingsdb-1",
        "port": 9220,
        "stream": "<node-out:1> 10.0.0.11:9220"
    },
    {
        "node_id": 2,
        "syntax_version": "v0",
        "status": "READY",
        "zone": 0,
        "committed_event_id": 5,
        "stored_event_id": 4,
        "next_thing_id": 6,
        "node_name": "thingsdb-2",
        "port": 9220,
        "stream": "<node-out:2> 10.0.0.12:9220"
    }
]
```

## Schedule backups

Now we are ready to use ThingsDB but before actually creating collections we are gonna configure backup scheduling like we promised.

In this example we will create a daily backup schedule on node 0 and a weekly schedule on node 1. This is just an example and both schedules
could be running on the same node, or more or different schedules may be planned as you like.

### Create a daily backup schedule

We will keep our daily backups for 14 days, so after two weeks the backup file will be removed from Google Cloud Storage by ThingsDB automatically.

```bash
curl --location --request POST 'http://localhost:9210/node/0' \
--header 'Content-Type: application/json' \
--user admin:pass \
--data-raw '{
    "type": "query",
    "code": "new_backup('gs://thingsdb-backups/node0_{DATE}_{TIME}.tar.gz', '2000-01-01 2:00', 3600*24, 14);"
}'
```

This will immediately create a backup since we have chosen a start time in the past (2000-01-01...). Once the backup is made, the next backup will be scheduled at 2:00 AM and will repeat every 24 hours.
As a scope we used `/node/0` which tells to run the query on node with id 0.


### Create a weekly backup schedule

To create weekly backups we do almost the same as above but change the repeat time to 7 days and we start at `2000-01-02 2:00`. This happens to be a sunday so the weekly backups will be made each sunday at 2:00 AM. You do not have to worry about having two schedules on sunday since ThingsDB will not run the backups simultaneous thus always keeps at least two nodes available for other work. This means that one of the two backups will be created after the other one is finished. Instead of 14 files we chose to keep 52 files so we will keep weekly backups for one full year.

```bash
curl --location --request POST 'http://localhost:9210/node/1' \
--header 'Content-Type: application/json' \
--user admin:pass \
--data-raw '{
    "type": "query",
    "code": "new_backup('gs://thingsdb-backups/node1_{DATE}_{TIME}.tar.gz', '2000-01-02 2:00', 3600*24*7, 52);"
}'
```

## More...

It is also a good idea to remove the default `admin` password and switch to token authentication.
Documentation on how to create a token can be found here: https://docs.thingsdb.net/v0/connect/authentication/#token-authentication