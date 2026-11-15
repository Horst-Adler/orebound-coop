"""Native choice-row contract, including regressions that must be rejected.

Does not launch Gothic or replace the Union runtime parser.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = (ROOT / "scripts/OreboundCoop_Menu.d").read_text(encoding="ascii")
FIELDS = ("ROLE", "GENDER", "LOG", "ENGINE", "FF", "LANGUAGE")

def check(script):
    definitions = {name: (parent, body) for _, name, parent, body in re.findall(
        r"(prototype|instance)\s+(\w+)\s*\((\w+)\)\s*\{(.*?)\};", script, re.S | re.I)}
    menu = definitions["MENU_ZWEIKLANG_COOP"][1]
    slots = [(int(slot), name) for slot, name in re.findall(r'items\[(\d+)\]\s*=\s*"([^"\n]+)"', menu)]
    assert len({slot for slot, _ in slots}) == len(slots), "duplicate slots"
    names = [name for _, name in sorted(slots)]
    assert len(set(names)) == len(names), "duplicate entries"
    label = definitions["ZK_MENU_CHOICE_LABEL"][1]
    assert re.search(r"flags\s*=\s*IT_SELECTABLE\s*\|\s*IT_EFFECTS_NEXT", label), "label must forward"
    assert re.search(r"onselaction\[0\]\s*=\s*SEL_ACTION_UNDEF", label), "label must not close menu"
    assert re.search(r"flags\s*=\s*flags\s*&\s*~IT_SELECTABLE", definitions["ZK_MENU_CHOICE"][1]), "duplicate choice navigation"
    last_y = -1
    for name in names:
        match = re.search(r"posy\s*=\s*(\d+)", definitions[name][1])
        if match:
            y = int(match[1])
            assert y >= last_y, "navigation order"
            last_y = y
    for field in FIELDS:
        left, right = "MENUITEM_ZK_" + field + "_LABEL", "MENUITEM_ZK_" + field + "_CHOICE"
        assert definitions[left][0] == "ZK_MENU_CHOICE_LABEL", "unselectable label"
        assert definitions[right][0] == "ZK_MENU_CHOICE", "wrong value type"
        assert names[names.index(left) + 1] == right, "wrong native forwarding target"
    # Native Up/Down traverses slots; IT_EFFECTS_NEXT addresses the next slot.
    # Each label is one navigation stop and its choice is its Left/Right target.
    row_order = [name for name in names if name.endswith("_LABEL") and name.split("_")[-2] in FIELDS]
    assert row_order == ["MENUITEM_ZK_" + field + "_LABEL" for field in FIELDS], "choice row order"
    assert names.index("MENUITEM_ZK_LANGUAGE_CHOICE") < names.index("MENUITEM_ZK_APPLY"), "save precedes choices"

class NativeChoiceRows(unittest.TestCase):
    def test_current_menu(self):
        check(SCRIPT)
    def test_labels_must_receive_input(self):
        with self.assertRaises(AssertionError):
            check(SCRIPT.replace("flags = IT_SELECTABLE | IT_EFFECTS_NEXT;", "flags = IT_SELECTABLE;"))
    def test_language_label_must_be_selectable(self):
        with self.assertRaises(AssertionError):
            check(SCRIPT.replace("MENUITEM_ZK_LANGUAGE_LABEL(ZK_MENU_CHOICE_LABEL)", "MENUITEM_ZK_LANGUAGE_LABEL(ZK_MENU_LABEL)"))
    def test_choices_must_precede_actions(self):
        with self.assertRaises(AssertionError):
            check(SCRIPT.replace('items[23] = "MENUITEM_ZK_LANGUAGE_LABEL"', 'items[23] = "MENUITEM_ZK_APPLY"').replace('items[25] = "MENUITEM_ZK_APPLY"', 'items[25] = "MENUITEM_ZK_LANGUAGE_LABEL"'))
    def test_label_enter_must_not_go_back(self):
        with self.assertRaises(AssertionError):
            check(SCRIPT.replace("onselaction[0] = SEL_ACTION_UNDEF;", "onselaction[0] = SEL_ACTION_BACK;"))

if __name__ == "__main__":
    unittest.main()
