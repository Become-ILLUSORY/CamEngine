#!/usr/bin/env python3
"""
JNI 符号一致性校验工具
比对 Kotlin external fun 与 C++ JNIEXPORT 符号名是否匹配，预防闪退。
用法: python3 scripts/check_jni.py
退出码: 0=全匹配 1=有不匹配
"""
import re, sys, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KOTLIN = os.path.join(ROOT, "android", "app", "src", "main", "kotlin")
CPP = os.path.join(ROOT, "android", "app", "src", "main", "cpp")
PKG_PREFIX = "com_illusory_camengine_"

def parse_kotlin():
    """返回 [(jni_sig_expected, file)]：Kotlin 顶层函数→类名带Kt，类内函数→用类名"""
    expected = []
    for dirpath, _, files in os.walk(KOTLIN):
        for fn in files:
            if not fn.endswith(".kt"): continue
            full = os.path.join(dirpath, fn)
            with open(full) as f: lines = f.readlines()
            # 类名跟踪（支持嵌套）
            stack = []
            for ln in lines:
                stripped = ln.strip()
                m = re.match(r'(?:class|object)\s+([A-Za-z0-9_]+)', stripped)
                if m:
                    stack.append(m.group(1))
                for cls in list(stack):
                    if cls in stripped and stripped.rstrip().endswith('}'):
                        pass
                if 'external fun ' in stripped:
                    # 提取方法名
                    meth = stripped.split('external fun ')[1]
                    meth = re.split(r'[\(:]', meth)[0].strip()
                    if stack:
                        full_cls = '_'.join(stack)
                        expected.append(("Java_%s_%s" % (PKG_PREFIX + full_cls, meth), full, stack[:]))
                    else:
                        # 顶层函数 → 文件名 + Kt
                        basename = fn.replace('.kt','') + 'Kt'
                        expected.append(("Java_%s_%s_%s" % (PKG_PREFIX, basename, meth), full, ['<top>']))
    return expected

def parse_cpp():
    symbols = []
    for dirpath, _, files in os.walk(CPP):
        for fn in files:
            if not (fn.endswith(".cpp") or fn.endswith(".cc") or fn.endswith(".c")): continue
            with open(os.path.join(dirpath, fn)) as f:
                for ln in f:
                    for sym in re.findall(r'Java_[A-Za-z0-9_]+', ln):
                        symbols.append(sym)
    return set(symbols)

def main():
    expected = parse_kotlin()
    symbols = parse_cpp()
    print("=== C++ 已实现的 JNI 符号 ===")
    for s in sorted(symbols): print("  ", s)
    print("\n=== Kotlin 期望的 JNI 符号 ===")
    errors = 0
    for sig, fname, cls in expected:
        ok = sig in symbols
        if not ok: errors += 1
        print("  [%s] %s" % ("OK " if ok else "FAIL", sig))
    print("\n结果: %d 个预期符号, %d 个不匹配" % (len(expected), errors))
    sys.exit(1 if errors else 0)

if __name__ == "__main__":
    main()
