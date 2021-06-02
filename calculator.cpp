#include <iostream>
#include <string>
#include <vector>

enum class TokenType { ADD, SUB, MUL, DIV, LITERAL, RIGHT_PAREN, LEFT_PAREN };

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

	void Scan() {
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
				size_t start = position;
				bool hasDecimalPoint = false;
				while (IsDigit(source[position]) || (!hasDecimalPoint && source[position] == '.')) {
					if (source[position] == '.') {
						hasDecimalPoint = true;
					}
					position++;
				}
				float value = std::stof(source.substr(start, position - start));
				tokens.emplace_back(value);
				break;
			}
		}
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
};

class Expression {
public:
	virtual float Evaluate() = 0;
	virtual ~Expression() {}
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
	AddExpression(Expression* lhs, Expression* rhs) 
	: BinaryExpression(lhs, rhs) {
	}

	float Evaluate() {
		return lhs->Evaluate() + rhs->Evaluate();
	}
};

class SubtractExpression : public BinaryExpression {
public:
	SubtractExpression(Expression* lhs, Expression* rhs) 
	: BinaryExpression(lhs, rhs) {
	}

	float Evaluate() {
		return lhs->Evaluate() - rhs->Evaluate();
	}
};

class MultiplyExpression : public BinaryExpression {
public:
	MultiplyExpression(Expression* lhs, Expression* rhs) 
	: BinaryExpression(lhs, rhs) {
	}

	float Evaluate() {
		return lhs->Evaluate() * rhs->Evaluate();
	}
};

class DivideExpression : public BinaryExpression {
public:
	DivideExpression(Expression* lhs, Expression* rhs) 
	: BinaryExpression(lhs, rhs) {
	}

	float Evaluate() {
		return lhs->Evaluate() / rhs->Evaluate();
	}
};

class Parser {
public:
	Parser(const std::vector<Token>& tokens) : position(0), tokens(tokens) {
	}

	Expression* Parse() {
		return Term();
	}
private:
	size_t position;
	const std::vector<Token>& tokens;

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
		if (Match(TokenType::LITERAL)) {
			float value = tokens[position].literal_value;
			position++;
			return new LiteralExpression{value};
		}

		if (Match(TokenType::LEFT_PAREN)) {
			position++;
			Expression* expr = Term();
			Consume(TokenType::RIGHT_PAREN);
			return expr;
		}
	}
};

int main() {
	Lexer lexer("(3 + 3)  * 2/ (4 -1)");
	lexer.Scan();

	Parser parser(lexer.GetTokens());
	Expression* expr = parser();
	std::cout << expr->Evaluate();

	delete expr;
}