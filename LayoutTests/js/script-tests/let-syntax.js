description('Tests for ES6 "let"');

function truth() { return true; }
noInline(truth);

function assert(cond) {
    if (!cond)
        throw new Error("Broke assertion");
}

function hasSyntaxError(str) {
    let hadError = false;
    try {
        eval(str);
    } catch(e) {
        if (e.name === "SyntaxError") {
            hadError = true;
        }
    }
    return hadError; 
}

function shouldHaveSyntaxError(str) {
    assert(hasSyntaxError(str));
    assert(hasSyntaxError("function dummy() { " + str + " }"));
    testPassed("Has syntax error: '" + str + "'");
    str = "'use strict'; " + str;
    assert(hasSyntaxError(str));
    assert(hasSyntaxError("function dummy() { " + str + " }"));
    testPassed("Has syntax error: '" + str + "'");
}


function shouldNotHaveSyntaxError(str) {
    assert(!hasSyntaxError(str));
    assert(!hasSyntaxError("(function dummy() { " + str + " })"));
    testPassed("Does not have syntax error: '" + str + "'");
    str = "'use strict'; " + str;
    assert(!hasSyntaxError(str));
    assert(!hasSyntaxError("(function dummy() { " + str + " })"));
    testPassed("Does not have syntax error: '" + str + "'");
}

function shouldHaveSyntaxErrorStrictOnly(str)
{
    assert(!hasSyntaxError(str));
    assert(!hasSyntaxError("(function dummy() { " + str + " })"));
    testPassed("Does not have syntax error: '" + str + "'");
    str = "'use strict'; " + str;
    assert(hasSyntaxError(str));
    assert(hasSyntaxError("(function dummy() { " + str + " })"));
    testPassed("Has syntax error: '" + str + "'");
}

shouldNotHaveSyntaxError("let x = 20; if (truth()) { let x = 30; }");
shouldNotHaveSyntaxError("let {x} = {x:20}; if (truth()) { let {x} = {x : 20}; }");
shouldNotHaveSyntaxError("let {x} = {x:20}; if (truth()) { let {y: x} = {y : 20}; }");
shouldNotHaveSyntaxError("let {x, y: [arr]} = {x:20, y: [10]}; if (truth()) { let {y: x} = {y : 20}; }");
shouldNotHaveSyntaxError("let i = 40; for (let i = 1; i < 2; i++) { let i = 40; i; }");
shouldNotHaveSyntaxError("let i = 40; let obj = {}; for (let i in obj) { let i = 40; let obj = {}; i; }");
shouldNotHaveSyntaxError("let i = 40; let obj = []; for (let i of obj) { let i = 40; let obj = {}; i; }");
shouldNotHaveSyntaxError("let {i} = 20; let obj = []; for (let {i} of obj) { let i = 40; let obj = {}; i; }");
shouldNotHaveSyntaxError("let {i} = 20; let obj = []; for (let {i} in obj) { let i = 40; let obj = {}; i; }");
shouldNotHaveSyntaxError("let {i} = 20; let obj = []; for (let {i} = {i: 0}; i < 2; i++) { let i = 40; let obj = {}; i; }");
shouldNotHaveSyntaxError("function foo() { let foo = 20; }");
shouldNotHaveSyntaxError("function foo(bar) { if (truth()) { let bar; } }");
shouldNotHaveSyntaxError("function foo() { var bar; if (truth()) { let bar; } }");
shouldNotHaveSyntaxError(";({ get let() { return 50; }, set let(x) { return 50;} });");

shouldHaveSyntaxError("let let;");
shouldHaveSyntaxError("const let;");
shouldHaveSyntaxError("let {let};");
shouldHaveSyntaxError("let {l: let};");
shouldHaveSyntaxError("let {l: {let}};");
shouldHaveSyntaxError("let {l: [let]};");
shouldHaveSyntaxError("var {let};");
shouldHaveSyntaxError("let x, x;");
shouldHaveSyntaxError("let x = 20, y, x = 40;");
shouldHaveSyntaxError("let x = 20, y; let x = 40;");
shouldHaveSyntaxError("let x = 20, y, {x} = {};");
shouldHaveSyntaxError("let x = 20, y; let {x} = {};");
shouldHaveSyntaxError("let {x, y, z, x} = {};");
shouldHaveSyntaxError("let {x: xx, y, x: xx} = {};");
shouldHaveSyntaxError("let {x: xx,  foo: [xx]} = {foo:[12]};");
shouldHaveSyntaxError("let {x: xx,  foo: {y: xx}} = {foo:[12]};");
shouldHaveSyntaxError("for (let; ; ) {}");
shouldHaveSyntaxError("let arr = []; for (let    of arr) {}");
shouldHaveSyntaxError("let obj = {}; for (let    in arr) {}");
shouldHaveSyntaxError("for (let i = 20, j = 40, i = 10; i < 10; i++) {}");
shouldHaveSyntaxError("let x = 20; if (truth()) let x = 40;");
shouldHaveSyntaxError("let baz = 20; if (truth()) { let x = 20; let x = 40;} ");
shouldHaveSyntaxError("function foo() { var bar; let bar; }");
shouldHaveSyntaxError("function foo(bar) { let bar; }");
shouldHaveSyntaxError("function foo() {}; let foo;");
shouldHaveSyntaxError("function foo() {}; function bar(){} let baz, {bar} = {};");
shouldHaveSyntaxError("function foo() {}; function bar(){} let baz, {f: {bar}} = {f:{}};");
shouldHaveSyntaxError("function foo() {}; function bar(){} let baz, {f: [bar]} = {f:[10]};");
shouldHaveSyntaxError("for (let let = 0; let < 10; let++) {}");
shouldHaveSyntaxError("for (let of []) {}");
shouldHaveSyntaxError("for (let in {}) {}");

// Stay classy, ES6.
shouldHaveSyntaxErrorStrictOnly("let;");
shouldHaveSyntaxErrorStrictOnly("var let;");
shouldHaveSyntaxErrorStrictOnly("var {let} = 40;");
shouldHaveSyntaxErrorStrictOnly("var [let] = 40;");
shouldHaveSyntaxErrorStrictOnly("var {p: let} = 40;");
shouldHaveSyntaxErrorStrictOnly("(function test(let){});");
shouldHaveSyntaxErrorStrictOnly("let: for (v of []) break let;");
shouldHaveSyntaxErrorStrictOnly("let: for (v of []) continue let;");
shouldHaveSyntaxErrorStrictOnly("let: for (v in {}) break;");
shouldHaveSyntaxErrorStrictOnly("let: for (v in {}) break;");
shouldHaveSyntaxErrorStrictOnly("let: for (var v = 0; false; ) {};");
shouldHaveSyntaxErrorStrictOnly("try { } catch(let) {}");
