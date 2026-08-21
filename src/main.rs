use ignore::Walk;
use tree_sitter::{Parser, Query, QueryCursor};

mod build_graph;

fn main() {
    let mut c_parser = Parser::new();
    let language = tree_sitter_c::LANGUAGE.into();

    c_parser
        .set_language(&language)
        .expect("Failed loading C lang");

    match Query::new(&language, QUERY) {
        Ok(query) => {
            let mut cursor = QueryCursor::new();

            for entry in Walk::new(".").filter_map(|e| e.ok()) {
                if let file = entry.path().display().to_string().as_str()
                    && file.ends_with(".c")
                {
                    let src = std::fs::read_to_string(file).expect("Error opening C file");
                    let tree = c_parser
                        .parse(&src, None)
                        .expect("Error parsing the C code");

                    let funcs = build_graph::enumeration_pass(&src, &tree, &mut cursor, &query,
                        &file);
                }
            }
        },
        Err(e) => {
            println!("{}", e.message);
        }
    }
}

const QUERY: &'static str =  r#"
(function_definition
    (storage_class_specifier) @fn.storage
    (#eq? @fn.storage "static")
    declarator: [
        (function_declarator
            declarator: (identifier) @fn.name)
        (pointer_declarator
            declarator: (function_declarator
            declarator: (identifier) @fn.name))
    ]
) @fn.is_static

(function_definition
    (storage_class_specifier)? @fn.storage
    (#not-eq? @fn.storage "static")
    declarator: [
        (function_declarator
            declarator: (identifier) @fn.name)
        (pointer_declarator
            declarator: (function_declarator
            declarator: (identifier) @fn.name))
    ]
) @fn.is_non_static
"#;
