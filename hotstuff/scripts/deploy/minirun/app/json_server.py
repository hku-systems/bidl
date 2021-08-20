#!/usr/bin/python3

import os
import subprocess

ANSIBLE_METADATA = {
    'metadata_version': '0.1',
    'status': ['preview'],
    'supported_by': 'Ted Yin'
}

DOCUMENTATION = '''
---
module: json_server

short_description: Ansible module for an example app (json-server)

version_added: "2.9"

options:
    config:
        description: Path to config file
        required: false
        type: str
    port:
        description: Set port
        required: false
        type: int
    host:
        description: Set host
        required: false
        type: str
    db:
        description: database file
        required: true
        type: str
    logdir:
        description: log dir
        required: false
        type: str

author:
    - Ted Yin (@Tederminant)
'''

EXAMPLES = '''
'''

RETURN = '''
'''

from ansible.module_utils.basic import AnsibleModule

def run_module():
    # define available arguments/parameters a user can pass to the module
    module_args = dict(
        bin=dict(
            type='str',
            default='json-server'),
        cwd=dict(
            type='str',
            default='~/'),
        config=dict(type='str', required=False),
        port=dict(type='int', required=False),
        host=dict(type='str', required=False),
        db=dict(type='str', required=True),
        logdir=dict(type='str', required=False)
    )

    module = AnsibleModule(
        argument_spec=module_args,
        supports_check_mode=False
    )

    json_server_args = [
        'config',
        'port',
        'host',
    ]

    expanduser = [
        'bin',
        'cwd',
        'config',
        'db',
        'logdir',
    ]

    for arg in expanduser:
        module.params[arg] = os.path.expanduser(module.params[arg])

    try:
        cmd = [*(module.params['bin'].split())]
        for arg in json_server_args:
            val = module.params[arg]
            if not (val is None):
                _arg = "--{}".format(arg.replace('_', '-'))
                if type(val) != str or len(val) > 0:
                    cmd.append("{}={}".format(_arg, val))
        cmd.append(module.params['db'])
        logdir = module.params['logdir']
        if not (logdir is None):
            stdout = open(os.path.expanduser(
                os.path.join(logdir, 'stdout')), "w")
            stderr = open(os.path.expanduser(
                os.path.join(logdir, 'stderr')), "w")
            nullsrc = open("/dev/null", "r")
        else:
            (stdout, stderr) = None, None

        cwd = module.params['cwd']

        pid = subprocess.Popen(
                cmd,
                cwd=cwd,
                stdin=nullsrc,
                stdout=stdout, stderr=stderr,
                env=os.environ,
                shell=False,
                start_new_session=True).pid
        module.exit_json(
                changed=False,
                status=0, pid=pid, cmd=" ".join(cmd), cwd=cwd)
    except (OSError, subprocess.SubprocessError) as e:
        module.fail_json(msg=str(e), changed=False, status=1)


def main():
    run_module()


if __name__ == '__main__':
    main()
