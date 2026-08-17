use tree_sitter::{Parser, Query, QueryCursor};
use ignore::Walk;

mod get_captures;

fn main() {
    let mut c_parser = Parser::new();
    let language = tree_sitter_c::LANGUAGE.into();

    c_parser
        .set_language(&language)
        .expect("Failed loading C lang");

    let query_str = r#"
    (function_definition
        declarator: [
            (function_declarator
                declarator: (identifier) @function.name
                parameters: (parameter_list) @function.args)
            (pointer_declarator
                declarator: (function_declarator
                    declarator: (identifier) @function.name
                    parameters: (parameter_list) @function.args))
        ]
        body: (compound_statement) @function.body
    ) @function.def
"#;

    let query = Query::new(&language, query_str).unwrap();
    let mut cursor = QueryCursor::new();

    for entry in Walk::new(".").filter_map(|e| e.ok()) {
        if let name = entry.path().display().to_string()
            && name.ends_with(".c")
        {
            println!("{name}:\n==================================");
            let src = std::fs::read_to_string(name).expect("Error opening C file");
            let tree = c_parser
                .parse(&src, None)
                .expect("Error parsing the C code");

            get_captures::print_function_map(&src, &tree, &mut cursor, &query);
        }
    }
}
