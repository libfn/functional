---
title: fn::pack
---

##### Defined in {style: "api", badge: "#include <functional/pack.hpp>"}

---

<!--
For some reason `fn::pack` does not work here and produces the following error:
```
Exception in thread "main" java.lang.RuntimeException: 
markup parsing error:
    TOC item: reference-manual/pack
    full path: /Users/godexsoft/Development/functional/docs/reference-manual/pack.md

error handling include plugin <doxygen-doc>
  free param: fn::pack
  opts: {}

expected to find element <declname>

        at org.testingisdocumenting.znai.website.WebSite.throwParsingErrorMessage(WebSite.java:570)
        at org.testingisdocumenting.znai.website.WebSite.parseMarkupAndUpdateTocItemAndSearch(WebSite.java:562)
        at java.base/java.util.ArrayList.forEach(ArrayList.java:1597)
        at java.base/java.util.Collections$UnmodifiableCollection.forEach(Collections.java:1117)
        at org.testingisdocumenting.znai.website.WebSite.parseMarkups(WebSite.java:501)
        at org.testingisdocumenting.znai.website.WebSite.parse(WebSite.java:193)
        at org.testingisdocumenting.znai.website.WebSite.parseAndDeploy(WebSite.java:179)
        at org.testingisdocumenting.znai.website.WebSite$Configuration.deployTo(WebSite.java:1034)
        at org.testingisdocumenting.znai.cli.ZnaiCliApp.generateDocs(ZnaiCliApp.java:197)
        at org.testingisdocumenting.znai.cli.ZnaiCliApp.start(ZnaiCliApp.java:99)
        at org.testingisdocumenting.znai.cli.ZnaiCliApp.start(ZnaiCliApp.java:70)
        at org.testingisdocumenting.znai.cli.ZnaiCliApp.main(ZnaiCliApp.java:75)
Caused by: java.lang.RuntimeException: error handling include plugin <doxygen-doc>
  free param: fn::pack
  opts: {}

expected to find element <declname>

        at org.testingisdocumenting.znai.parser.commonmark.MarkdownVisitor.handleIncludePlugin(MarkdownVisitor.java:232)
        at org.testingisdocumenting.znai.parser.commonmark.MarkdownVisitor.visit(MarkdownVisitor.java:155)
        at znaishaded.org.commonmark.node.CustomBlock.accept(CustomBlock.java:7)
        at znaishaded.org.commonmark.node.AbstractVisitor.visitChildren(AbstractVisitor.java:137)
        at znaishaded.org.commonmark.node.AbstractVisitor.visit(AbstractVisitor.java:28)
        at znaishaded.org.commonmark.node.Document.accept(Document.java:7)
        at org.testingisdocumenting.znai.parser.commonmark.MarkdownParser.parsePartial(MarkdownParser.java:98)
        at org.testingisdocumenting.znai.parser.commonmark.MarkdownParser.parse(MarkdownParser.java:66)
        at org.testingisdocumenting.znai.website.WebSite.parseMarkupAndUpdateTocItemAndSearch(WebSite.java:549)
        ... 10 more
Caused by: java.lang.IllegalArgumentException: expected to find element <declname>
        at org.testingisdocumenting.znai.utils.XmlUtils.lambda$nextLevelNodeByName$1(XmlUtils.java:87)
        at java.base/java.util.Optional.orElseThrow(Optional.java:403)
        at org.testingisdocumenting.znai.utils.XmlUtils.nextLevelNodeByName(XmlUtils.java:87)
        at org.testingisdocumenting.znai.doxygen.parser.DoxygenMemberParser.lambda$parseMember$0(DoxygenMemberParser.java:63)
        at java.base/java.util.stream.SpinedBuffer$1Splitr.forEachRemaining(SpinedBuffer.java:364)
        at java.base/java.util.stream.ReferencePipeline$Head.forEach(ReferencePipeline.java:782)
        at org.testingisdocumenting.znai.doxygen.parser.DoxygenMemberParser.parseMember(DoxygenMemberParser.java:62)
        at org.testingisdocumenting.znai.doxygen.parser.DoxygenMemberParser.parseXml(DoxygenMemberParser.java:44)
        at org.testingisdocumenting.znai.doxygen.parser.DoxygenMemberParser.parse(DoxygenMemberParser.java:38)
        at org.testingisdocumenting.znai.doxygen.parser.DoxygenCompoundParser.parseMember(DoxygenCompoundParser.java:66)
        at java.base/java.util.stream.SpinedBuffer$1Splitr.forEachRemaining(SpinedBuffer.java:356)
        at java.base/java.util.stream.ReferencePipeline$Head.forEach(ReferencePipeline.java:782)
        at org.testingisdocumenting.znai.doxygen.parser.DoxygenCompoundParser.parseXml(DoxygenCompoundParser.java:62)
        at org.testingisdocumenting.znai.doxygen.parser.DoxygenCompoundParser.parse(DoxygenCompoundParser.java:39)
        at org.testingisdocumenting.znai.doxygen.Doxygen.findAndParseCompound(Doxygen.java:136)
        at org.testingisdocumenting.znai.doxygen.Doxygen.getCachedOrFindAndParseCompound(Doxygen.java:101)
        at org.testingisdocumenting.znai.doxygen.plugin.DoxygenDocIncludePlugin.process(DoxygenDocIncludePlugin.java:58)
        at org.testingisdocumenting.znai.extensions.TrackingIncludePlugin.process(TrackingIncludePlugin.java:65)
        at org.testingisdocumenting.znai.parser.commonmark.MarkdownVisitor.handleIncludePlugin(MarkdownVisitor.java:228)
        ... 18 more
```

Interestingly using fn::detail::pack_impl does not produce the same error. 
And similarly looking code in choice.md and sum.md also appears to work fine.
-->
:include-doxygen-doc: fn::detail::pack_impl

---

## Return value {style: "api"}
TODO

