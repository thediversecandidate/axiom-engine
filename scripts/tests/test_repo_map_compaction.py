"""gen_repo_map.py: special-member compaction keeps every non-boilerplate constructor."""
from gen_repo_map import summarize_special


def test_move_only_class_collapses_to_one_line():
    sigs = ["Foo() = default", "Foo(Foo &&other) noexcept", "Foo &operator=(Foo &&other) noexcept",
            "Foo(const Foo &) = delete", "Foo &operator=(const Foo &) = delete", "~Foo()", "int size() const"]
    assert summarize_special("Foo", sigs) == ["(move-only, default-constructible)", "int size() const"]


def test_meaningful_constructor_is_kept():
    sigs = ["Foo(int capacity)", "Foo(const Foo &) = delete", "~Foo()"]
    out = summarize_special("Foo", sigs)
    assert "Foo(int capacity)" in out and out[0] == "(non-copyable)"


def test_class_without_special_members_is_unchanged():
    assert summarize_special("Foo", ["int a", "void f()"]) == ["int a", "void f()"]
