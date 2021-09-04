
from sshtunnel import open_tunnel
import paramiko
from paramiko import SSHClient
from scp import SCPClient

file_list = ["/home/sosp21ae/bidl/figure"]

private_key = """-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAwA/6jdbRBTqPc2rLKo562ndBGBfggEgCyyEt1h1p6fubPbM0
3NEQwIkzJzZ0kIgFcfvBMdaGIEqMYt0yaVNOBulibf48dvx0QpKkk5BIAREQxOeh
5cy/pzFFFuoe1S0NMhjVQVQdG8z+5CcFKttL8KLm3WK58lE8u6mnmk4Jwu/yHa0K
ZLl8pEgxUAoRHD/GM2mVfWHHwABRzEzHU+mg+dc8MgiQvcYR/YppPkOHfcRSGnFw
+T9qUCw0wc2RHHaUaubX1F2dgvKRTx+x3aERejM9jJHPq8tiFAOTE8+/fa+CXf4X
LwFB+WtyFkmhkc9KQflHtE3NdIJyB/4v+V3LVwIDAQABAoIBADcNHXNEjLsj8vRR
OxTirCIsppLiXS5H9c0FoJ7L5Yz02qWP1Wop3tRhPzFRMY4v/ueSEulXJTDiTWUV
JBr+jtH6WYVPp0Mvz585a0UEyS7NFnJqNoSo8JJa1APibZ4LAMpIRfAozQMn+jOp
X5jOLPE3sFbqsoPbdz4PPgRDtBaUs0ByFY+wIbB5xWlRTJ9xbI6x9OLb59xCDxrI
ipjwOcIThDyZOGYd/qAbnZPh1m3eOn2r+NyNdZ+IFPjoPgWCfIK7v3BFqeUHJ2S8
wEAli6KZN6Hgmm1iicfLrUpIjua43hrZ8BkvDYk02VWpbAjEQVNOJUBgixtnoMvl
cnLJiEECgYEA4FPshFVoL2ZRhZp3SJDTcka5L/6MzXeIGbr0rsuFITeRGDrzjIeY
g78lk2tbZYoWPyMHzJuwMWu2AKjoqU3B4WtzufF9Gd4ttXkgzTulk2/ThLAvES6D
DFnB/CLvwkNzEl6y+j3vS6xVZEFFBsHGmvOllshNLWO0PU6ERkDmWLcCgYEA2y3b
pKFbuM/3b3kHUPobuAUTcyA3X/jiQLJEYnwdwsNcSBPYJ4DW/qipMnIkn2MUPM4y
EuhlZVVHB86GGOKHC6TxfGG82QbOsTySJIWOsLGFpTjHlhwvLO9KLIxphPUeWXXl
ehYww9VfnCsizuxSTSnymTBH7vVO2bzPzg2BQmECgYEAkP3Ndm8ZauGQOXFwPcfV
0xvhICM/8KGcpzzOX5gN0T1wG6AmwKL3sde/Orx+E3AgFujWZqoORLMgrOZ/ksY+
VSu1Xe629BxFoPDMgbpWt6fkp/OUSF+utjHhjs0p+H7OBoaM2e9kFp4phyYkrkxa
yNZIJeAK+hZibems1xelipsCgYEAps2bnwr+FwSOttNQ8humBMI4LyGRgwSSi9If
oSP8MkEjdedEVawD8wLA4Zdkr7hjqnLyQNmEcT4ZNKPEAiRDymTZM2PeqPw5SUFj
p0/NbENuBnPT0TXZ4USrGoQxufIiCstivSsuKZu1i9B4SSoo6HjIHDVWSVF/lM3R
CGvWt4ECgYAOuMHCYBtdaYmkWsRL/MoOgACIsKGeTfp5/c+Y7YFrZbfpjbEI+T0g
086IzJuSWHTz51VseptNFmE0QuE3YAPX2SotNWlu9ZnKzeTeUWP8aHuHdzD1ss3f
M+bcIID4L9kiUZ2gJlRLfjD8M3QFUykr66JB9QUwLcFpe/fHhJCIoA==
-----END RSA PRIVATE KEY-----"""

with open("pk.key.tmp", "w") as f:
    f.write(private_key)

with open_tunnel(
    ('gatekeeper3.cs.hku.hk', 22),
    ssh_username="sosp2021",
    ssh_private_key="pk.key.tmp",
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