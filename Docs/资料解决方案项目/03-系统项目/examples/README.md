# 示例配置

此目录包含客户接入示例。

## 使用方式

```bash
# 从示例输入构建知识库
doc-solution build-kb --input ./examples/customer-huawei/ --name "华为-HarmonyOS"

# 检查示例文档
doc-solution check --target ./examples/sample-docs/

# 使用示例模板生成内容
doc-solution generate --template api-ref --params '{"api_name": "startAbility"}'
```
