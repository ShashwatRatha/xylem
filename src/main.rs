use tree_sitter::{Parser, Query,
        QueryCursor, StreamingIterator};

fn main() {
    let mut c_parser = Parser::new();

    c_parser.set_language(&tree_sitter_c::LANGUAGE.into())
        .expect("Failed loading C lang");

    let src = std::fs::read_to_string("src/main.c")
        .expect("Error opening C file");
    let tree = c_parser.parse(&src, None).unwrap();

    let query_str = "
    (function_definition
        declarator: (function_declarator
            declarator: (identifier) @function)
        body: (_) @func_body)
    ";

    let query = Query::new(&tree_sitter_c::LANGUAGE.into(), query_str).unwrap();
    let mut cursor = QueryCursor::new();

    let mut matches = cursor.matches(&query, tree.root_node(), src.as_bytes());

    while let Some(m) = matches.next() {
        let mut name = "";
        let mut body = "";

        for capture in m.captures {
            let capture_name = &query.capture_names()[capture.index as usize];
            let text = capture.node.utf8_text(src.as_bytes()).unwrap();

            let func_def_range = capture.node.range();

            match &capture_name[..] {
                "function" => name = text,
                "func_body" => body = text,
                _ => ()
            }

            if !name.is_empty() && !body.is_empty() {
                println!("Captured function {} from start: {} and end: {} with body:\n{}\n", name,
                    func_def_range.start_point, func_def_range.end_point, body);
            }
        }
    }
}
