#!/usr/bin/env python3
"""
契约驱动开发流水线 — CLI 入口

面向 ArkTS + QT 混合应用的多阶段、多 Worker 代码生成与维护流水线。
将自然语言需求转化为可生成、可构建、可验证的代码，自带自愈闭环。

用法：
    # 运行完整流水线
    python main.py run "新增文本处理功能：QT 后端处理文本，ArkTS 展示结果"

    # 查看流水线运行状态
    python main.py status <pipeline_id>

    # 列出最近运行记录
    python main.py list

    # 查看记忆/已学模式
    python main.py memory list

    # 搜索相似历史案例
    python main.py memory search "文本处理"

    # 中止正在运行的流水线
    python main.py abort <pipeline_id>

    # 配置 LLM 后端
    python main.py config set llm.backend openai
"""

import argparse
import json
import logging
import os
import sys
import datetime
from pathlib import Path
from typing import List, Optional

sys.path.insert(0, str(Path(__file__).parent))

from pipeline.orchestrator import PipelineOrchestrator
from pipeline.memory import PipelineMemory
from pipeline.knowledge_base import KnowledgeBase

logger = logging.getLogger(__name__)


def setup_cli() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="契约驱动开发流水线 — ArkTS + QT 混合应用代码生成",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例：
  python main.py run "新增文本输入回显功能：QT 接收文本，通过 NAPI 返回 ArkTS 显示"
  python main.py run --config my_config.yaml "自定义配置运行"
  python main.py status pipe-a1b2c3d4e5f6
  python main.py memory list
  python main.py memory search "qt napi bridge"
        """,
    )

    subparsers = parser.add_subparsers(dest="command", help="可用命令")

    run_parser = subparsers.add_parser("run", help="运行完整流水线")
    run_parser.add_argument("requirement", type=str, help="自然语言需求描述")
    run_parser.add_argument("--config", "-c", type=str, default="config/pipeline.yaml",
                            help="流水线配置文件路径")
    run_parser.add_argument("--output", "-o", type=str, default="",
                            help="流水线结果输出目录")
    run_parser.add_argument("--verbose", "-v", action="store_true",
                            help="启用详细日志")

    status_parser = subparsers.add_parser("status", help="查看流水线运行状态")
    status_parser.add_argument("pipeline_id", type=str, help="流水线运行 ID")

    list_parser = subparsers.add_parser("list", help="列出最近运行记录")
    list_parser.add_argument("--limit", "-n", type=int, default=10,
                             help="显示条数")

    memory_parser = subparsers.add_parser("memory", help="记忆与知识管理")
    memory_sub = memory_parser.add_subparsers(dest="memory_command")

    memory_list = memory_sub.add_parser("list", help="列出记忆条目")
    memory_search = memory_sub.add_parser("search", help="搜索记忆条目")
    memory_search.add_argument("query", type=str, help="搜索关键词")
    memory_show = memory_sub.add_parser("show", help="查看记忆条目详情")
    memory_show.add_argument("entry_id", type=str, help="记忆条目 ID")

    config_parser = subparsers.add_parser("config", help="配置管理")
    config_sub = config_parser.add_subparsers(dest="config_command")
    config_set = config_sub.add_parser("set", help="设置配置项")
    config_set.add_argument("key", type=str, help="配置键（点号分隔）")
    config_set.add_argument("value", type=str, help="配置值")

    abort_parser = subparsers.add_parser("abort", help="中止正在运行的流水线")
    abort_parser.add_argument("pipeline_id", type=str, help="流水线运行 ID")

    return parser


def cmd_run(args: argparse.Namespace) -> int:
    print(f"\n{'='*60}")
    print(f"  契约驱动开发流水线")
    print(f"  配置文件: {args.config}")
    print(f"{'='*60}\n")

    print(f"需求: {args.requirement}\n")

    orchestrator = PipelineOrchestrator(args.config)
    context = orchestrator.run(args.requirement)

    print(f"\n{'='*60}")
    print(f"  流水线完成: {context.status.value.upper()}")
    print(f"  ID: {context.pipeline_id}")
    print(f"  修复迭代次数: {context.repair_iterations}")

    if context.build_results:
        for br in context.build_results:
            print(f"  构建 [{br.stage}]: {'✅ 通过' if br.success else '❌ 失败'} ({br.duration_seconds:.1f}s)")

    if context.verification_result:
        vr = context.verification_result
        print(f"  测试: {vr.passed}/{vr.total_tests} 通过")

    print(f"{'='*60}\n")

    return 0 if context.status.value == "passed" else 1


def cmd_status(args: argparse.Namespace) -> int:
    archive_dir = Path("archive") / args.pipeline_id
    if archive_dir.exists():
        print(f"流水线: {args.pipeline_id}")
        for f in archive_dir.iterdir():
            print(f"  {f.name}")
    else:
        print(f"流水线 '{args.pipeline_id}' 未找到归档")
        return 1
    return 0


def cmd_list(args: argparse.Namespace) -> int:
    archive_dir = Path("archive")
    if not archive_dir.exists():
        print("暂无流水线运行记录")
        return 0

    runs = sorted(archive_dir.iterdir(), key=lambda p: p.stat().st_mtime, reverse=True)
    print(f"最近运行记录（显示 {min(len(runs), args.limit)} 条）：\n")
    for run_dir in runs[:args.limit]:
        mod_time = datetime.datetime.fromtimestamp(run_dir.stat().st_mtime)
        print(f"  {run_dir.name}  ({mod_time.strftime('%Y-%m-%d %H:%M:%S')})")
    return 0


def cmd_memory(args: argparse.Namespace) -> int:
    memory = PipelineMemory()

    if args.memory_command == "list":
        entries = memory.list_entries()
        print(f"记忆条目 ({len(entries)} 条)：\n")
        for entry in entries:
            tags = f" [{', '.join(entry.tags)}]" if entry.tags else ""
            print(f"  {entry.entry_id}: {entry.title}{tags}")

    elif args.memory_command == "search":
        results = memory.find_similar(args.query)
        print(f"搜索 '{args.query}' 的结果：\n")
        for entry in results:
            print(f"  {entry.entry_id}: {entry.title}")
            if entry.key_lessons:
                for lesson in entry.key_lessons[:2]:
                    print(f"    └ {lesson}")

    elif args.memory_command == "show":
        entry = memory.get_entry(args.entry_id)
        if entry:
            print(f"条目: {entry.entry_id}")
            print(f"标题: {entry.title}")
            print(f"需求: {entry.requirement}")
            print(f"\n关键经验:")
            for lesson in entry.key_lessons:
                print(f"  - {lesson}")
            print(f"\n标签: {', '.join(entry.tags)}")
            print(f"创建时间: {entry.created_at}")
        else:
            print(f"未找到条目: {args.entry_id}")
            return 1

    else:
        print("用法: python main.py memory [list|search|show]")
        return 1

    return 0


def cmd_config(args: argparse.Namespace) -> int:
    if args.config_command == "set":
        print(f"配置 {args.key} = {args.value}")
        print("（配置持久化功能尚未实现 — 请直接修改 config/pipeline.yaml）")
    return 0


def cmd_abort(args: argparse.Namespace) -> int:
    print(f"已请求中止流水线: {args.pipeline_id}")
    print("（中止信号需要运行中的流水线 — 按需实现 IPC）")
    return 0


def main() -> int:
    parser = setup_cli()
    args = parser.parse_args()

    if args.command == "run":
        return cmd_run(args)
    elif args.command == "status":
        return cmd_status(args)
    elif args.command == "list":
        return cmd_list(args)
    elif args.command == "memory":
        return cmd_memory(args)
    elif args.command == "config":
        return cmd_config(args)
    elif args.command == "abort":
        return cmd_abort(args)
    else:
        parser.print_help()
        return 0


if __name__ == "__main__":
    sys.exit(main())
