#include <iostream>
#include <string>
#include <string_view>
#include <vector>

class Error {
public:
	enum class Type { NO_ERROR, INVALID_CHAR, MISSING_CLOSING_PAREN, MISSING_OPENING_PAREN };

	Error(Error::Type type, size_t location, std::string_view source) : type(type), location(location), source(source) {
	}

	Type type;
	size_t location;
	std::string_view source;

	friend std::ostream& operator<<(std::ostream& out, Error error) {
		if (error.type == Type::INVALID_CHAR) {
			out << "Error: Unexpected Character: '" << error.source[error.location] << "'\n";
			out << "    " << error.source << "\n";
			out << std::string(error.location + 4, ' ') << "^---- Here";
		}

		return out;
	}

	operator bool() {
		return type != Type::NO_ERROR;
	}
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
				position++;
				break;
			case '-':
				tokens.emplace_back(Token::Type::SUB);
				position++;
				break;
			case '*':
				tokens.emplace_back(Token::Type::MUL);
				position++;
				break;
			case '/':
				tokens.emplace_back(Token::Type::DIV);
				position++;
				break;
			case '(':
				tokens.emplace_back(Token::Type::LEFT_PAREN);
				position++;
				break;
			case ')':
				tokens.emplace_back(Token::Type::RIGHT_PAREN);
				position++;
				break;
			default:
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

	void PrintTokens() {
		for (auto& token : tokens) {
			std::cout << '|';
			switch (token.token_type) {
			case Token::Type::ADD:
				std::cout << '+';
				break;
			case Token::Type::SUB:
				std::cout << '-';
				break;
			case Token::Type::MUL:
				std::cout << '*';
				break;
			case Token::Type::DIV:
				std::cout << '/';
				break;
			case Token::Type::LEFT_PAREN:
				std::cout << '(';
				break;
			case Token::Type::RIGHT_PAREN:
				std::cout << ')';
				break;
			case Token::Type::LITERAL:
				std::cout << token.literal_value;
				break;
			}
		}
		std::cout << '|' << std::endl;
	}

	const std::vector<Token>& GetTokens() {
		return tokens;
	}

private:
	size_t position;
	std::vector<Token> tokens;
	const std::string source;

private:
	bool IsDigit(char ch) {
		return ch >= '0' && ch <= '9';
	}

	bool IsWhiteSpace(char ch) {
		return ch == ' ' || ch == '\t';
	}

	std::pair<float, bool> GetLiteral() {
		if (!IsDigit(source[position])) { // ".234" is considered an error
			return {0, false};
		}
		size_t start = position;
		bool hasDecimalPoint = false;

		while (IsDigit(source[position]) || (!hasDecimalPoint && source[position] == '.')) {
			if (source[position] == '.') {
				hasDecimalPoint = true;
			}
			position++;
		}

		return {std::stof(source.substr(start, position - start)), true};
	}
};

class Expression {
public:
	virtual float Evaluate() = 0;
	virtual ~Expression() {
	}
};

class LiteralExpression : public Expression {
public:
	LiteralExpression(float value) : value(value) {
	}
	float Evaluate() {
		return value;
	}

	float value;
};

class BinaryExpression : public Expression {
public:
	BinaryExpression(Expression* lhs, Expression* rhs) : lhs(lhs), rhs(rhs) {
	}

	virtual ~BinaryExpression() {
		delete lhs;
		delete rhs;
	}

	Expression* lhs;
	Expression* rhs;
};

class AddExpression : public BinaryExpression {
public:
	AddExpression(Expression* lhs, Expression* rhs) : BinaryExpression(lhs, rhs) {
	}

	float Evaluate() {
		return lhs->Evaluate() + rhs->Evaluate();
	}
};

class SubtractExpression : public BinaryExpression {
public:
	SubtractExpression(Expression* lhs, Expression* rhs) : BinaryExpression(lhs, rhs) {
	}

	float Evaluate() {
		return lhs->Evaluate() - rhs->Evaluate();
	}
};

class MultiplyExpression : public BinaryExpression {
public:
	MultiplyExpression(Expression* lhs, Expression* rhs) : BinaryExpression(lhs, rhs) {
	}

	float Evaluate() {
		return lhs->Evaluate() * rhs->Evaluate();
	}
};

class DivideExpression : public BinaryExpression {
public:
	DivideExpression(Expression* lhs, Expression* rhs) : BinaryExpression(lhs, rhs) {
	}

	float Evaluate() {
		return lhs->Evaluate() / rhs->Evaluate();
	}
};

class Parser {
public:
	Parser(const std::vector<Token>& tokens) : position(0), tokens(tokens), expr(nullptr) {
	}

	~Parser() {
		if (expr)
			delete expr;
	}

	Error Parse() {
		expr = Term();
		return Error(Error::Type::NO_ERROR, 0, "");
	}

	float Value() {
		if (expr)
			return expr->Evaluate();

		return 0;
	}

private:
	size_t position;
	const std::vector<Token>& tokens;
	Expression* expr;

private:
	template <typename... Args>
	bool Match(Token::Type type, Args... args) {
		return Check(type) || Match(args...);
	}

	bool Match(Token::Type type) {
		return Check(type);
	}

	bool Check(Token::Type type) {
		if (IsAtEnd())
			return false;

		return tokens[position].token_type == type;
	}

	bool IsAtEnd() {
		return position == tokens.size();
	}

	bool Consume(Token::Type type) {
		position++;
		return tokens[position - 1].token_type == type;
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
	}
};

void ProcessInput(const std ::string& input) {
	Lexer lexer(input);
	Error error = lexer.Scan();

	if (error) {
		std::cout << error << std::endl;
		return;
	}

	Parser parser(lexer.GetTokens());
	error = parser.Parse();

	if (error) {
		std::cout << error << std::endl;
		return;
	}

	std::cout << parser.Value() << std::endl;
}

void PrintInfo() {
	std::cout << "Basic CLI calculator by Jun Lim https://github.com/junnys6018" << std::endl;
	std::cout << "Type 'exit' to exit\n";
}

int main() {
	PrintInfo();
	std::string input;

	std::cout << ">>> ";
	while (std::getline(std::cin, input)) {
		if (input == "exit") {
			return 0;
		}
		ProcessInput(input);
		std::cout << ">>> ";
	}
}

// 3a
// 23.23.3

// 3++3
// ()
// ())
// (
// (2+1))
// 23 23