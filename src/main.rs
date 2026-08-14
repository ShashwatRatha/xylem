use std::collections::HashMap;

use tree_sitter::{Node, Parser, Query,
    QueryCursor, StreamingIterator};
use walkdir::WalkDir;

fn main() {
    let mut c_parser = Parser::new();
    let language = tree_sitter_c::LANGUAGE.into();

    c_parser
        .set_language(&language)
        .expect("Failed loading C lang");

    let query_str = "
    (call_expression
        function: [
            (identifier) @callee
            (field_expression field: (field_identifier) @callee)
        ])
    ";

    let dir = WalkDir::new(".").into_iter().filter_map(|e| e.ok());
    for entry in dir {
        if let name = entry.path().display().to_string()
            && !name.starts_with("./.")
                && !name.starts_with("./target")
                && name.ends_with(".c") {
                    let src = std::fs::read_to_string(name).expect("Error opening C file");
                    let tree = c_parser.parse(&src, None).unwrap();
                    let query = Query::new(&language, query_str).unwrap();
                    let mut cursor = QueryCursor::new();

                    let mut matches = cursor.matches(&query, tree.root_node(), src.as_bytes());

                    let mut map: HashMap<String, Vec<String>> = HashMap::new();

                    while let Some(m) = matches.next() {
                        for capture in m.captures {
                            let capture_name = &query.capture_names()[capture.index as usize];
                            if *capture_name == "callee" 
                                && let Ok(callee) = capture.node.utf8_text(src.as_bytes()) {
                                    if let Some(enclosing_fn) = find_enclosing_fn(capture.node) 
                                        && let Some(caller_name) = find_caller_name(enclosing_fn, src.as_bytes()) {
                                            map.entry(caller_name.to_string())
                                                .or_default()
                                                .push(callee.to_string());
                                    }

                                }
                        }
                    }

                    for (caller, callees) in map {
                        println!("{caller} calls:");
                        for callee in callees {
                            print!("{callee} ");
                        }
                        println!("\n");
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
