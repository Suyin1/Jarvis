# Should Fail - Try Used

This code block has try:

```ts
function doSomething() {
  try {
    doSthSync1();
  } catch (error) {
    console.error("Failed:", error);
  }
}
```

Another try block:

```ts
try {
  await doSthAsync1();
} catch (err) {
  console.error(err);
}
```
