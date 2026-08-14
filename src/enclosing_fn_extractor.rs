use tree_sitter::Node;

pub fn find_enclosing_fn<'a>(mut node: Node<'a>) -> Option<Node<'a>> {
    while let Some(parent) = node.parent() {
        if parent.kind() == "function_definition" {
            return Some(parent);
        }
        node = parent;
    }
    None
}

pub fn find_caller_name<'a>(func_def_node: Node<'a>, src: &'a [u8]) -> Option<&'a str> {
    let mut n = func_def_node.child_by_field_name("declarator")?;

    loop {
        if n.kind() == "identifier" {
            return n.utf8_text(src).ok();
        }

        if let Some(child) = n.child_by_field_name("declarator") {
            n = child;
        } else if let Some(child) = n.named_child(0) {
            n = child;
        } else {
            break;
        }
    }

    None
}
