#include <algorithm>
#include <iostream>
#include <locale>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

class Error {
public:
	enum class Type { NO_ERROR, INVALID_CHAR, INVALID_TOKEN, END_OF_STREAM };

	Error(Error::Type type, size_t location, std::string_view source) : type(type), location(location), source(source) {
	}

	operator bool() {
		return type != Type::NO_ERROR;
	}

	friend std::ostream& operator<<(std::ostream& out, Error error) {
		if (error.type == Type::INVALID_CHAR) {
			out << "Error: Unexpected Character: '" << error.source[error.location] << "'\n";
		} else if (error.type == Type::INVALID_TOKEN) {
			out << "Error: Unexpected Token\n";
		} else if (error.type == Type::END_OF_STREAM) {
			out << "Error: Unexpected End Of Stream\n";
		}

		out << "    " << error.source << "\n";
		out << std::string(error.location + 4, ' ') << "^---- Here";

		return out;
	}

	Type type;
	size_t location;
	std::string_view source;
};

class Token {
public:
	enum class Type { ADD, SUB, MUL, DIV, LITERAL, RIGHT_PAREN, LEFT_PAREN };
	Token(Type type) : token_type(type) {
	}

	Token(float value) : token_type(Type::LITERAL), literal_value(value) {
	}

	Type token_type;
	float literal_value;
};

class Lexer {
public:
	Lexer(const std::string& source) : position(0), tokens(), source(source) {
	}

	Error Scan() {
		while (position < source.length()) {
			if (IsWhiteSpace(source[position])) {
				position++;
				continue;
			}
			switch (source[position]) {
			case '+':
				tokens.emplace_back(Token::Type::ADD);
				token_positions.push_back(position);
				position++;
				break;
			case '-':
				tokens.emplace_back(Token::Type::SUB);
				token_positions.push_back(position);
				position++;
				break;
			case '*':
				tokens.emplace_back(Token::Type::MUL);
				token_positions.push_back(position);
				position++;
				break;
			case '/':
				tokens.emplace_back(Token::Type::DIV);
				token_positions.push_back(position);
				position++;
				break;
			case '(':
				tokens.emplace_back(Token::Type::LEFT_PAREN);
				token_positions.push_back(position);
				position++;
				break;
			case ')':
				tokens.emplace_back(Token::Type::RIGHT_PAREN);
				token_positions.push_back(position);
				position++;
				break;
			default:
				token_positions.push_back(position);
				auto [value, success] = GetLiteral();
				if (!success) {
					return Error(Error::Type::INVALID_CHAR, position, source);
				}
				tokens.emplace_back(value);
				break;
			}
		}
		return Error(Error::Type::NO_ERROR, 0, source);
	}

	const std::vector<Token>& GetTokens() {
		return tokens;
	}

	const std::vector<size_t>& GetPositions() {
		return token_positions;
	}

private:
	size_t position;
	std::vector<Token> tokens;
	std::vector<size_t> token_positions;
	const std::string& source;

private:
	bool IsDigit(char ch) {
		return ch >= '0' && ch <= '9';
	}

	bool IsWhiteSpace(char ch) {
		return std::isspace(ch);
	}

	std::pair<float, bool> GetLiteral() {
		if (!IsDigit(source[position])) { // ".234" is considered an error
			return {0, false};
		}
		size_t start = position;
		bool has_decimal_point = false;

		while (IsDigit(source[position]) || (!has_decimal_point && source[position] == '.')) {
			if (source[position] == '.') {
				has_decimal_point = true;
			}
			position++;
		}

		// I believe string::substr() makes a copy, probs not a good idea here as
		// we only want to parse a float from a string
		return {std::stof(source.substr(start, position - start)), true};
	}
};

class Expression {
public:
	virtual float Evaluate() const = 0;
	virtual ~Expression() {
	}
};

class LiteralExpression : public Expression {
public:
	LiteralExpression(float value) : value(value) {
	}
	float Evaluate() const override {
		return value;
	}

	float value;
};

class BinaryExpression : public Expression {
public:
	BinaryExpression(Expression* lhs, Expression* rhs) : lhs(lhs), rhs(rhs) {
	}

	std::unique_ptr<Expression> lhs;
	std::unique_ptr<Expression> rhs;
};

class AddExpression : public BinaryExpression {
public:
	AddExpression(Expression* lhs, Expression* rhs) : BinaryExpression(lhs, rhs) {
	}

	float Evaluate() const override {
		return lhs->Evaluate() + rhs->Evaluate();
	}
};

class SubtractExpression : public BinaryExpression {
public:
	SubtractExpression(Expression* lhs, Expression* rhs) : BinaryExpression(lhs, rhs) {
	}

	float Evaluate() const override {
		return lhs->Evaluate() - rhs->Evaluate();
	}
};

class MultiplyExpression : public BinaryExpression {
public:
	MultiplyExpression(Expression* lhs, Expression* rhs) : BinaryExpression(lhs, rhs) {
	}

	float Evaluate() const override {
		return lhs->Evaluate() * rhs->Evaluate();
	}
};

class DivideExpression : public BinaryExpression {
public:
	DivideExpression(Expression* lhs, Expression* rhs) : BinaryExpression(lhs, rhs) {
	}

	float Evaluate() const override {
		return lhs->Evaluate() / rhs->Evaluate();
	}
};

class Parser {
public:
	Parser(const std::vector<Token>& tokens, const std::vector<size_t>& token_positions, const std::string& source)
		: position(0), tokens(tokens), token_positions(token_positions), source(source) {
	}

	Error Parse() {
		try {
			expr = std::shared_ptr<Expression>(Term());
		} catch (int) {
			if (IsAtEnd()) {
				return Error(Error::Type::END_OF_STREAM, source.size(), source);
			}

			return Error(Error::Type::INVALID_TOKEN, token_positions[position], source);
		}

		if (position != tokens.size()) {
			return Error(Error::Type::INVALID_TOKEN, token_positions[position], source);
		}
		return Error(Error::Type::NO_ERROR, 0, source);
	}

	std::shared_ptr<Expression> GetAST() {
		return expr;
	}

private:
	size_t position;
	const std::vector<Token>& tokens;
	const std::vector<size_t>& token_positions;
	const std::string& source;
	std::shared_ptr<Expression> expr;

private:
	template <typename... Args>
	bool Match(Token::Type first, Args... args) {
		return Check(first) || Match(args...);
	}

	bool Match(Token::Type first) {
		return Check(first);
	}

	bool Check(Token::Type type) {
		if (IsAtEnd())
			return false;

		return tokens[position].token_type == type;
	}

	bool IsAtEnd() {
		return position == tokens.size();
	}

	void Consume(Token::Type type) {
		if (tokens[position].token_type != type) {
			throw 1;
		}
		position++;
	}

	Expression* Term() {
		Expression* expr = Factor();

		while (Match(Token::Type::ADD, Token::Type::SUB)) {
			Token::Type type = tokens[position].token_type;
			position++;
			Expression* rhs = Factor();
			if (type == Token::Type::ADD) {
				expr = new AddExpression{expr, rhs};
			} else {
				expr = new SubtractExpression{expr, rhs};
			}
		}

		return expr;
	}

	Expression* Factor() {
		Expression* expr = Primary();

		while (Match(Token::Type::MUL, Token::Type::DIV)) {
			Token::Type type = tokens[position].token_type;
			position++;
			Expression* rhs = Primary();
			if (type == Token::Type::MUL) {
				expr = new MultiplyExpression{expr, rhs};
			} else {
				expr = new DivideExpression{expr, rhs};
			}
		}

		return expr;
	}

	Expression* Primary() {
		if (Match(Token::Type::LITERAL)) {
			float value = tokens[position].literal_value;
			position++;
			return new LiteralExpression{value};
		}

		if (Match(Token::Type::LEFT_PAREN)) {
			position++;
			Expression* expr = Term();
			Consume(Token::Type::RIGHT_PAREN);
			return expr;
		}

		throw 1;
	}
};

void ProcessInput(const std ::string& input) {
	Lexer lexer(input);
	Error error = lexer.Scan();

	if (error) {
		std::cout << error << std::endl;
		return;
	}

	Parser parser(lexer.GetTokens(), lexer.GetPositions(), input);
	error = parser.Parse();

	if (error) {
		std::cout << error << std::endl;
		return;
	}

	std::cout << parser.GetAST()->Evaluate() << std::endl;
}

void PrintInfo() {
	std::cout << "Basic CLI calculator by Jun Lim https://github.com/junnys6018" << std::endl;
	std::cout << "Type 'exit' to exit\n";
}

void Trim(std::string& s) {
	// Trim from the left
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));

	// Trim from the right
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

int main() {
	PrintInfo();
	std::string input;

	std::cout << ">>> ";
	while (std::getline(std::cin, input)) {
		Trim(input);

		if (input == "exit") {
			return 0;
		} else if (input == "") {
			std::cout << ">>> ";
			continue;
		}
		ProcessInput(input);
		std::cout << ">>> ";
	}
}