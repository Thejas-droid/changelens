#!/usr/bin/env python3
from pathlib import Path
import random, sys
root=Path(sys.argv[1] if len(sys.argv)>1 else 'demo/testgraph-demo-commerce')
random.seed(42)
modules=['auth','users','catalog','cart','orders','payments','notifications','inventory','common']
if root.exists():
    import shutil; shutil.rmtree(root)
for m in modules:
    (root/'src'/m).mkdir(parents=True,exist_ok=True)
    (root/'tests'/m).mkdir(parents=True,exist_ok=True)
# 25 sources/module = 225 source nodes.
for mi,m in enumerate(modules):
    for i in range(25):
        p=root/'src'/m/f'{m}_{i}.hpp'
        includes=[]
        if i>0: includes.append(f'{m}/{m}_{i-1}.hpp')
        if m!='common': includes.append(f'common/common_{i%25}.hpp')
        if mi>0 and i%5==0:
            prev=modules[mi-1]; includes.append(f'{prev}/{prev}_{(i+3)%25}.hpp')
        text='#pragma once\n'+''.join(f'#include "{x}"\n' for x in includes)+f'inline int {m}_{i}() {{ return {mi*100+i}; }}\n'
        p.write_text(text)
# explicit cycle fixture.
(root/'src/common/cycle_a.hpp').write_text('#pragma once\n#include "common/cycle_b.hpp"\n')
(root/'src/common/cycle_b.hpp').write_text('#pragma once\n#include "common/cycle_a.hpp"\n')
# 18 tests for first 8 modules + 16 common = 160 tests.
count=0
for m in modules:
    n=16 if m=='common' else 18
    for i in range(n):
        src=i%25
        p=root/'tests'/m/f'{m}_{i}_test.cpp'
        body=f'#include "{m}/{m}_{src}.hpp"\nTEST({m.title()}, Case{i}) {{}}\n'
        if m=='notifications' and i==0:
            body+='// KNOWN_FLAKY: demo fixture for retry classification\n'
        if i==1:
            body+='// SLOW_TEST: demo scheduler fixture\n'
        p.write_text(body)
        count+=1
(root/'testgraph.yml').write_text('''version: 1
project:
  name: testgraph-demo-commerce
  roots:
    - src
tests:
  patterns:
    - "tests/**/*_test.cpp"
  alwaysRun:
    - "tests/auth/**"
execution:
  workers: 8
cache:
  enabled: true
flaky:
  enabled: true
  maxRetries: 2
fallback:
  fullSuiteWhenChanged:
    - "CMakeLists.txt"
    - "testgraph.yml"
ignore:
  - ".git/**"
''')
(root/'README.md').write_text(f'# ChangeLens demo commerce fixture\n\nGenerated deterministic analysis fixture with {len(modules)*25+2} source nodes and {count} test files across commerce-style modules. It includes deep dependencies, shared common dependencies, an explicit cycle, slow-test markers, and one known-flaky marker.\n')
print(root, 'sources',len(modules)*25+2,'tests',count)
