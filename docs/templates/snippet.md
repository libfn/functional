<#if desc??>
```columns
left:
</#if>
:include-file: ${path} {
<#if surroundedBy??>
  surroundedBy: ["${surroundedBy?join("\", \"")}"],
</#if>
  surroundedBySeparator: ["\n...\n"],
  commentsType: "inline"
}
<#if desc??>
right:
${desc}
```
</#if>
