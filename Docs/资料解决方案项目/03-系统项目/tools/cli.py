"""CLI 主入口

定义 doc-solution 命令组，注册所有子命令。
使用 click 库实现命令行接口。
"""

import click

from tools.check import check_command
from tools.generate import generate_command
from tools.build_kb import build_kb_command
from tools.test_rule import test_rule_command


@click.group()
@click.version_option(version="0.1.0", prog_name="doc-solution")
def main():
    """资料开发全链路解决方案工具集"""


main.add_command(check_command)
main.add_command(generate_command)
main.add_command(build_kb_command)
main.add_command(test_rule_command)


if __name__ == "__main__":
    main()
