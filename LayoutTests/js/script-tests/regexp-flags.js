description('Test RegExp#flags accessor');

debug("property descriptor");
var descriptor = Object.getOwnPropertyDescriptor(RegExp.prototype, 'flags');
shouldBe("descriptor.configurable", "true");
shouldBe("descriptor.enumerable", "false");
shouldBe("typeof descriptor.get", "'function'");
shouldBe("descriptor.set", "undefined");

var flags = descriptor.get;

shouldBe("/a/g.flags", "'g'");
shouldBe("/a/.flags", "''");
shouldBe("/a/gmi.flags", "'gim'"); // order is specified, happens to be alphabetic
shouldBe("new RegExp('a', 'gmi').flags", "'gim'");
shouldBe("flags.call(/a/ig)", "'gi'");

debug("non-object receivers");
shouldThrow("flags.call(undefined)", "'TypeError: Type error'");
shouldThrow("flags.call(null)", "'TypeError: Type error'");
shouldThrow("flags.call(false)", "'TypeError: Type error'");
shouldThrow("flags.call(true)", "'TypeError: Type error'");

debug("non-regex objects");
shouldBe("flags.call({})", "''");
shouldBe("flags.call({global: true, multiline: true, ignoreCase: true})", "'gim'");
shouldBe("flags.call({global: 1, multiline: 0, ignoreCase: 2})", "'gi'");
// inherited properties count
shouldBe("flags.call({ __proto__: { multiline: true } })", "'m'");

if (RegExp.prototype.hasOwnProperty('sticky')) {
  debug("sticky flag");
  // when the engine supports "sticky", these tests will fail by design.
  // Hopefully, only the expected output will need updating.
  shouldBe("/a/yimg.flags", "'gimy'");
  shouldBe("new RegExp('a', 'yimg').flags", "'gimy'");
  shouldBe("flags.call({global: true, multiline: true, ignoreCase: true, sticky: true})", "'gimy'");
}
if (RegExp.prototype.hasOwnProperty('unicode')) {
  debug("unicode flag");
  // when the engine supports "unicode", these tests will fail by design.
  // Hopefully, only the expected output will need updating.
  shouldBe("/a/uimg.flags", "'gimu'");
  shouldBe("new RegExp('a', 'uimg').flags", "'gimu'");
  shouldBe("flags.call({global: true, multiline: true, ignoreCase: true, unicode: true})", "'gimu'");
}
