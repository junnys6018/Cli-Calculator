#include <iostream>
#include <string>
#include <vector>

enum class TokenType { ADD, SUB, MUL, DIV, LITERAL, RIGHT_PAREN, LEFT_PAREN };

class Token {
public:
	Token(TokenType type) : token_type(type) {
	}

	Token(float value) : token_type(TokenType::LITERAL), literal_value(value) {
	}

	TokenType token_type;
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
				tokens.emplace_back(TokenType::ADD);
				position++;
				break;
			case '-':
				tokens.emplace_back(TokenType::SUB);
				position++;
				break;
			case '*':
				tokens.emplace_back(TokenType::MUL);
				position++;
				break;
			case '/':
				tokens.emplace_back(TokenType::DIV);
				position++;
				break;
			case '(':
				tokens.emplace_back(TokenType::LEFT_PAREN);
				position++;
				break;
			case ')':
				tokens.emplace_back(TokenType::RIGHT_PAREN);
				position++;
				break;
			default:
				size_t start = position;
				bool hasDecimalPoint = false;
				while (IsDigit(source[position]) || (!hasDecimalPoint && source[position] == '.')) {
					position++;
					if (source[position == '.']) {
						hasDecimalPoint = true;
					}
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
			case TokenType::ADD:
				std::cout << '+';
				break;
			case TokenType::SUB:
				std::cout << '-';
				break;
			case TokenType::MUL:
				std::cout << '*';
				break;
			case TokenType::DIV:
				std::cout << '/';
				break;
			case TokenType::LEFT_PAREN:
				std::cout << '(';
				break;
			case TokenType::RIGHT_PAREN:
				std::cout << ')';
				break;
			case TokenType::LITERAL:
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

class AddExpression : public Expression {
public:
	AddExpression(Expression* lhs, Expression* rhs) : lhs(lhs), rhs(rhs) {
	}
	float Evaluate() {
		return lhs->Evaluate() + rhs->Evaluate();
	}

	Expression* lhs;
	Expression* rhs;
};

class SubtractExpression : public Expression {
public:
	SubtractExpression(Expression* lhs, Expression* rhs) : lhs(lhs), rhs(rhs) {
	}
	float Evaluate() {
		return lhs->Evaluate() - rhs->Evaluate();
	}

	Expression* lhs;
	Expression* rhs;
};

class MultiplyExpression : public Expression {
public:
	MultiplyExpression(Expression* lhs, Expression* rhs) : lhs(lhs), rhs(rhs) {
	}
	float Evaluate() {
		return lhs->Evaluate() * rhs->Evaluate();
	}

	Expression* lhs;
	Expression* rhs;
};

class DivideExpression : public Expression {
public:
	DivideExpression(Expression* lhs, Expression* rhs) : lhs(lhs), rhs(rhs) {
	}
	float Evaluate() {
		return lhs->Evaluate() / rhs->Evaluate();
	}

	Expression* lhs;
	Expression* rhs;
};

class Parser {
public:
	Parser(const std::vector<Token>& tokens) : position(0), tokens(tokens) {
	}

	Expression* operator()() {
		return Term();
	}
private:
	size_t position;
	const std::vector<Token>& tokens;

private:
	template <typename... Args>
	bool Match(TokenType type, Args... args) {
		return Check(type) || Match(args...);
	}

	bool Match(TokenType type) {
		return Check(type);
	}

	bool Check(TokenType type) {
		if (IsAtEnd())
			return false;

		return tokens[position].token_type == type;
	}

	bool IsAtEnd() {
		return position == tokens.size();
	}

	bool Consume(TokenType type) {
		position++;
		return tokens[position - 1].token_type == type;
	}

	Expression* Term() {
		Expression* expr = Factor();

		while (Match(TokenType::ADD, TokenType::SUB)) {
			TokenType type = tokens[position].token_type;
			position++;
			Expression* rhs = Factor();
			if (type == TokenType::ADD) {
				expr = new AddExpression{expr, rhs};
			} else {
				expr = new SubtractExpression{expr, rhs};
			}
		}

		return expr;
	}

	Expression* Factor() {
		Expression* expr = Primary();

		while (Match(TokenType::MUL, TokenType::DIV)) {
			TokenType type = tokens[position].token_type;
			position++;
			Expression* rhs = Primary();
			if (type == TokenType::MUL) {
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
	Lexer lexer("(3+3)*2");
	lexer.Scan();
	lexer.PrintTokens();

	Parser parser(lexer.GetTokens());
	Expression* expr = parser();
	std::cout << expr->Evaluate();
}