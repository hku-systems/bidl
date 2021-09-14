
from sshtunnel import open_tunnel
import paramiko
import sys
from paramiko import SSHClient
from scp import SCPClient

file_list = ["/home/sosp21ae/bidl/figure"]

with open_tunnel(
    ('gatekeeper3.cs.hku.hk', 22),
    ssh_username="sosp2021",
    ssh_private_key=sys.argv[1],
    remote_bind_address=('202.45.128.161', 22)
) as server:
    print(server.local_bind_port)
    with SSHClient() as ssh:
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        ssh.connect('localhost',
            port=server.local_bind_port,
            username='sosp21ae',
            password='sosp21ae',
        )
        scp = SCPClient(ssh.get_transport())
        for file in file_list:
            scp.get(file, recursive=True)

print('FINISH!')