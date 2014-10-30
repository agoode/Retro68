#include "RezLexer.h"

#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>
#include <boost/wave/token_ids.hpp>
#include <boost/regex.hpp>

#include "RezLexerWaveToken.h"

namespace wave = boost::wave;

using namespace boost::wave;

static std::string readContents(std::istream&& instream)
{
	instream.unsetf(std::ios::skipws);

	return std::string(std::istreambuf_iterator<char>(instream.rdbuf()),
					   std::istreambuf_iterator<char>());
}

static std::string preFilter(std::string str)
{
	boost::regex endif("#endif[^\r\n]*");
	str = boost::regex_replace(str, endif, "#endif");

	boost::regex dollar_escape("\\\\\\$([a-zA-Z0-9][a-zA-Z0-9])");
	str = boost::regex_replace(str, dollar_escape, "\\\\0x$1");

	return str;
}

struct load_file_to_string_filtered
{
	template <typename IterContextT>
	class inner
	{
	public:
		template <typename PositionT>
		static void init_iterators(IterContextT &iter_ctx,
			PositionT const &act_pos, language_support language)
		{
			typedef typename IterContextT::iterator_type iterator_type;

			// read in the file
			std::ifstream instream(iter_ctx.filename.c_str());
			if (!instream.is_open()) {
				BOOST_WAVE_THROW_CTX(iter_ctx.ctx, preprocess_exception,
					bad_include_file, iter_ctx.filename.c_str(), act_pos);
				return;
			}

			iter_ctx.instring = preFilter(readContents(std::move(instream)));

			iter_ctx.first = iterator_type(
				iter_ctx.instring.begin(), iter_ctx.instring.end(),
				PositionT(iter_ctx.filename), language);
			iter_ctx.last = iterator_type();
		}

	private:
		std::string instring;
	};
};



typedef wave::cpplexer::lex_iterator<
		wave::cpplexer::lex_token<> >
	lex_iterator_type;
typedef wave::context<
		std::string::iterator, lex_iterator_type,
		load_file_to_string_filtered>
	context_type;
typedef context_type::iterator_type pp_iterator_type;

struct RezLexer::Priv
{
	std::string input;
	context_type ctx;
	pp_iterator_type iter;

	Priv(std::string data, std::string name)
		: input(data), ctx(input.begin(), input.end(), name.c_str())
	{
	}
};

RezLexer::RezLexer(std::string filename)
	: RezLexer(filename, readContents(std::ifstream(filename)))
{
}

RezLexer::RezLexer(std::string filename, const std::string &data)
{
	pImpl.reset(new Priv(preFilter(data), filename));

	pImpl->ctx.add_macro_definition("DeRez=0");
	pImpl->ctx.add_macro_definition("Rez=1");
	pImpl->ctx.add_macro_definition("true=1");
	pImpl->ctx.add_macro_definition("false=0");

	pImpl->iter = pImpl->ctx.begin();
}

RezLexer::~RezLexer()
{

}



void RezLexer::addDefine(std::string str)
{
	pImpl->ctx.add_macro_definition(str);
}

void RezLexer::addIncludePath(std::string path)
{
	pImpl->ctx.add_include_path(path.c_str());
}

bool RezLexer::atEnd()
{
	return pImpl->iter == pImpl->ctx.end();
}

RezLexer::WaveToken RezLexer::nextWave()
{
	if(pImpl->iter == pImpl->ctx.end())
		return WaveToken();
	else
	{
		WaveToken tok = *pImpl->iter++;
		return tok;
	}
}

RezLexer::WaveToken RezLexer::peekWave()
{
	return pImpl->iter == pImpl->ctx.end() ? WaveToken() : *pImpl->iter;
}
