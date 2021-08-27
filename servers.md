# How to access our servers for artifact evaluation. 

## Steps

#### Connect to HKU CS gatekeeper

For security reasons, our department only allow access to the gatekeeper using SSH keys. 

1. Create a key file and copy the private key from our HotCRP response into it. 

2. `chmod 400 [key_file_name]`. 

3. `ssh -i [key_file_name] sosp2021@gatekeeper3.cs.hku.hk`, and then enter the key passphrase (attached in our HotCRP response.)

#### Connect to our server

    ssh [server_account]@202.45.128.160

The server account and server password are listed in our HotCRP response. 

## Descriptions about our cluster. 


Each machine in our cluster has two IP addresses, one for access from the
gatekeeper, one for interconnection among the machines, as shown in the below
table. Please use the `202.45.128.xxx` address to SSH machines from the
gatekeeper, **and use the `10.22.1.x` for experiments (i.e., filling the `hosts.yml`
file)**.

| machine index | IP for access from the gatekeeper | IP for interconnection |
|---------------|-----------------------------------|------------------------|
| 1             | 202.45.128.160                    | 10.22.1.1              |
| 2             | 202.45.128.161                    | 10.22.1.2              |
| 3             | 202.45.128.162                    | 10.22.1.3              |
| ...           |                                   |                        |
| k             | 202.45.128.160+k-1                | 10.22.1.k              |


We have asked for access to 9 machines, but since the environment is not completely clean, 
the experiment results may have some outliers if other people happen to run tasks on the cluster at the same time.
You may re-run the outlier data points or simply ignore them when such contention happens. 

We have coordinated with other teams to better not use the **first 5 machines**
(i.e., `10.22.1.1~10.22.1.5`) in the next two weeks, so please better use
these 5 machines for most experiments to get a stable result. As explained
in the [artifact readme page](https://github.com/hku-systems/bidl), running the
experiments in a slightly smaller scale is already enough to verify our claims. 

## After accessing the cluster

We have helped to finish the setup for meeting prerequisite requirements and
creating the overlay network, so you can skip these steps in the instructions.

After cloning the codebase, you can start from filling the `hosts.yml` file
step. We have updated the readme file to mark the starting point.