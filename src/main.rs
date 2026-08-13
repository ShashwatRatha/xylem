use tree_sitter::{Node, Parser, Query, QueryCursor, StreamingIterator};

fn main() {
    let mut c_parser = Parser::new();
    let language = tree_sitter_c::LANGUAGE.into();

    c_parser
        .set_language(&language)
        .expect("Failed loading C lang");

    let src = std::fs::read_to_string("src/main.c").expect("Error opening C file");
    let tree = c_parser.parse(&src, None).unwrap();

    let query_str = "
    (call_expression
        function: [
            (identifier) @callee
            (field_expression field: (field_identifier) @callee)
        ])
    ";

    let query = Query::new(&language, query_str).unwrap();
    let mut cursor = QueryCursor::new();

    let mut matches = cursor.matches(&query, tree.root_node(), src.as_bytes());

    while let Some(m) = matches.next() {
        for capture in m.captures {
            let capture_name = &query.capture_names()[capture.index as usize];
            if *capture_name == "callee" 
                && let Ok(callee) = capture.node.utf8_text(src.as_bytes()) {
                    // Safely handle calls outside functions
                    if let Some(enclosing_fn) = find_enclosing_fn(capture.node) 
                        && let Some(caller_name) = find_caller_name(enclosing_fn, src.as_bytes()) {
                            println!("{caller_name} [Range: {} - {}] called: {callee}",
                                enclosing_fn.range().start_point, enclosing_fn.range().end_point);
                    }

                }
        }
    }
}

fn find_enclosing_fn<'a>(mut node: Node<'a>) -> Option<Node<'a>> {
    while let Some(parent) = node.parent() {
        if parent.kind() == "function_definition" {
            return Some(parent);
        }
        node = parent;
    }
    None
}

fn find_caller_name<'a>(func_def_node: Node<'a>, src: &'a [u8]) -> Option<&'a str> {
    let mut n = func_def_node.child_by_field_name("declarator")?;

    loop {
        if n.kind() == "identifier" {
            return n.utf8_text(src).ok();
        }

        // First attempt field-based traversal
        if let Some(child) = n.child_by_field_name("declarator") {
            n = child;
        } else if let Some(child) = n.named_child(0) {
            // Fall back to first named child for wrapper nodes (e.g. parenthesized declarators)
            n = child;
        } else {
            // Break loop safely if terminal node reached without finding identifier
            break;
        }
    }

    None
}
