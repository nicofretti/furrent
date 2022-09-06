#include "bencode_parser.hpp"

#include <regex>

using namespace fur::bencode;

std::string BencodeParser::encode(const BencodeValue& value) {
  return value.to_string();
}

auto BencodeParser::decode(const std::string& decoded) -> Result{
  _tokens = decoded;
  _index = 0;  // Reset the index
  auto r = decode();
  if(_index != _tokens.size()) {
    // The string was not fully parsed
    return Result::ERROR(BencodeParserError::InvalidString);
  }
  return r;
}

// Given a vector of tokens, returns a BencodeValue object
auto BencodeParser::decode() -> Result{
  if (!_tokens.empty()) {
    auto token = _tokens[_index];
    if (token == 'i') {
      return BencodeParser::decode_int();
    } else if (token<= '9' && token >= '0') {
      return BencodeParser::decode_string();
    } else if (token == 'l') {
      return BencodeParser::decode_list();
    } else if (token == 'd') {
      return BencodeParser::decode_dict();
    }
  }
  return Result::ERROR(BencodeParserError::InvalidString);
}

auto BencodeParser::decode_int() -> Result{
  // The token must be in the form ['i', 'number', 'e']
  if (_tokens.size() - _index < 3) {
    return Result::ERROR(BencodeParserError::IntFormat);
  }
  // Skip the 'i' token already checked before entering this function
  _index++;
  // Decoding the integer
  std::string integer{};
  while(_index < _tokens.size() && _tokens[_index] != 'e') {
    integer += _tokens[_index];
    _index++;
  }
  if (_index == _tokens.size()) {
    // No space for the 'e' token
    return Result::ERROR(BencodeParserError::IntFormat);
  }else if(_tokens[_index]!='e'){
    // Missing 'e' at the end of the integer
    return Result::ERROR(BencodeParserError::IntFormat);
  }else if (integer == "-0") {
    // The "-0" is not a valid integer
    return Result::ERROR(BencodeParserError::IntValue);
  }else if (!std::regex_match(integer, std::regex ("^-?\\d+$"))){
    // Check if the integer is a string of digits with sign
    return Result::ERROR(BencodeParserError::IntValue);
  }
  // Skip 'e' at the end
  _index++;
  return Result::OK(
      std::make_unique<BencodeInt>(std::stol(integer))
  );
}

auto BencodeParser::decode_string() -> Result {
  // The token must be in the form ['length', ':', 'string']
  if (_tokens.size() - _index < 3) {
    return Result::ERROR(BencodeParserError::StringFormat);
  }
  // Calculate the string length until the ':' token
  std::string len{};
  while(_index < _tokens.size() && _tokens[_index] != ':') {
    len += _tokens[_index];
    _index++;
  }
  if(len=="-0"){
    // The "-0" is not a valid integer
    return Result::ERROR(BencodeParserError::StringFormat);
  }else if(!std::regex_match(len, std::regex("^\\d+$"))) {
    // Check if the length is a positive integer
    return Result::ERROR(BencodeParserError::StringLength);
  }
  // Skip the ':' token
  _index++;
  // Calculate the string using the length previously calculated
  auto length_str = std::stol(len);
  std::string str{};
  for(int i = 0; i<length_str && _index < _tokens.size(); i++){
    str += _tokens[_index];
    _index++;
  }
  if(str.size() != static_cast<unsigned int>(length_str)) {
    // Exit because the string is not the same length as the given length
    return Result::ERROR(BencodeParserError::StringLength);
  }
  return Result::OK(std::make_unique<BencodeString>(str));
}

auto BencodeParser::decode_list() -> Result {
  // The token must be in the form ['l',...,'e']
  if (_tokens.size() - _index < 2) {
    return Result::ERROR(BencodeParserError::ListFormat);
  }
  auto ptr = std::vector<std::unique_ptr<BencodeValue>>();
  // Increment index to skip the first 'l' already checked before enter the
  // function
  _index += 1;
  while (_index < _tokens.size() && _tokens[_index] != 'e') {
    auto r = decode();
    if(!r){
      // An error occurred while decoding the list
      return r;
    }
    ptr.push_back(std::move(*r));
  }
  // Push all items but not space for 'e'
  if (_index >= _tokens.size()) {
    return Result::ERROR(BencodeParserError::ListFormat);
  }
  // Check if the list is closed with 'e'
  if (_tokens[_index] != 'e') {
    return Result::ERROR(BencodeParserError::ListFormat);
  }
  // Increment index to skip the last 'e'
  _index += 1;
  return Result::OK(std::make_unique<BencodeList>(std::move(ptr)));
}

auto BencodeParser::decode_dict() -> Result{
  // The token must be in the form ['d',...,'e']
  if (_tokens.size() - _index < 2) {
    return Result::ERROR(BencodeParserError::DictFormat);
  }
  // Increment index to skip the first 'd' already checked before enter the
  // function
  _index += 1;
  auto ptr = std::map<std::string, std::unique_ptr<BencodeValue>>();
  std::vector<std::string> keys = std::vector<std::string>();
  while (_index < _tokens.size() && _tokens[_index] != 'e') {
    auto r_key = decode();
    if(!r_key){
      // An error occurred while decoding a dictionary key
      return r_key;
    }
    auto key = std::move(*r_key);
    // The key must be a string
    if (key->get_type() != BencodeType::String) {
      return Result::ERROR(BencodeParserError::DictKey);
    }
    // Cast the key to a BencodeString
    auto r_value = BencodeParser::decode();
    if(!r_value){
      // An error occurred while decoding the value of a key
      return r_value;
    }
    auto key_str = dynamic_cast<BencodeString&>(*key).value();
    keys.push_back(key_str);
    ptr.insert({key_str, std::move(*r_value)});
  }
  // Push all items but not space for 'e'
  if (_index >= _tokens.size()) {
    return Result::ERROR(BencodeParserError::DictFormat);
  }
  // Check if the list is closed with 'e'
  if (_tokens[_index] != 'e') {
    return Result::ERROR(BencodeParserError::DictFormat);
  }
  // Check if the keys array is sorted by lexicographical order
  for (unsigned long i = 0; i < keys.size() - 1; i++) {
    if (keys[i] > keys[i + 1]) {
      return Result::ERROR(BencodeParserError::DictKeyOrder);
    }
  }
  // increment index to skip the last 'e'
  _index += 1;
  return Result::OK(std::make_unique<BencodeDict>(std::move(ptr)));
}
