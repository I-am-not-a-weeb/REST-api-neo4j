#include <iostream>
#include <cpprest/http_listener.h>
#include <cpprest/http_client.h>
//#include <boost/beast/core/detail/base64.hpp>

#include <cpprest/asyncrt_utils.h>

const utility::string_t authStr = L"bmVvNGo6dGVzdDEyMzQ=";
const utility::string_t url = L"http://localhost:7474/db/neo4j/tx/commit";

web::http::client::http_client* client;

void printJSON(web::json::value const& jvalue, utility::string_t const& prefix)
{
	std::wcout << prefix << jvalue.serialize() << std::endl;
}

bool isNumber(utility::string_t& str)
{
	for (char ch : str) {
		if (!isdigit(ch)) {
			return false;
		}
	}
	return true;
}

Concurrency::task<web::http::http_response> sendRequestGET(web::http::client::http_client client,web::json::object body)
{
	web::http::http_request request(web::http::methods::POST);
	request.headers().add(L"Authorization", authStr);
	request.headers().add(L"Content-Type", L"application/json");
	
	return client.request(request);
}

void handleRequest(web::http::http_request request, std::function<void(web::json::value const&, web::json::value&)> const& action)
{
	auto answer = web::json::value::object();
	request
		.extract_json()
		.then([&](pplx::task<web::json::value> task) 
		{
			try
			{
				const web::json::value& jvalue = task.get();

				printJSON(jvalue, L"R: ");
				if (!jvalue.is_null())
				{
					action(jvalue, answer);
				}
			}
			catch (web::http::http_exception const& e)
			{
				std::wcout << e.what() << std::endl;
				request.reply(web::http::status_codes::BadRequest, e.what());
			}
		})
		.wait();
	printJSON(answer, L"S: ");
	request.reply(web::http::status_codes::OK, answer);
}

void handleGET(web::http::http_request request);
void handlePOST(web::http::http_request request);
void handlePUT(web::http::http_request request);

int main()
{
	web::http::experimental::listener::http_listener listener(L"http://localhost:7475");
	client = new web::http::client::http_client(url);

	listener.support(web::http::methods::GET, std::bind(handleGET, std::placeholders::_1));
	listener.support(web::http::methods::POST, std::bind(handlePOST, std::placeholders::_1));
	listener.support(web::http::methods::PUT, std::bind(handlePUT, std::placeholders::_1));

   	/*web::http::http_request request(web::http::methods::POST);*/

	// bugi woogi
	//web::json::object* kek = &web::json::value::parse(L"{\"kek1\":1,\"kek2\":-1}").as_object();
	//web::json::object* kek = new web::json::object(web::json::value::parse(L"{\"kek1\":1,\"kek2\":-1}").as_object());

	/*request.headers().add(L"Authorization", authStr);
	request.headers().add(L"Content-Type", L"application/json");
	request.set_body(L"{\"statements\":[{\"statement\":\"MATCH (e:Employee) RETURN e\"}]}");*/

	//std::string toprint;
	//try
	//{
	//	client->request(request).then([&](web::http::http_response response)
	//		{
	//			try
	//			{
	//				std::cout << response.status_code() << std::endl;
	//				response.extract_json().then([&](web::json::value json)
	//					{
	//						toprint = utility::conversions::to_utf8string(json.serialize());
	//					}).wait();
	//			}
	//			catch (std::exception& e)
	//			{
	//				std::cout << e.what() << std::endl;
	//			}
	//		}).wait();
	//		std::cout << toprint << std::endl;
	//}
	//catch(std::exception& e)
	//{
	//			std::cout << e.what() << std::endl;
	//}
	


	try
	{
		listener.open()
			.then([&]() {std::wcout<< L"\nstarting to listen\n" ; })
			.wait();
		while (true);
	}
	catch (web::http::http_exception const& e)
	{
		std::wcout << e.what() << std::endl;
	}

	delete client;
}

void handleDELETE(web::http::http_request request)
{
		std::vector<utility::string_t>path = web::http::uri::split_path(web::http::uri::decode(request.relative_uri().path()));

	web::json::value answer = web::json::value::object();

	if (path.empty())
	{
		request.reply(web::http::status_codes::BadRequest, L"Bad request");
	}
	else if (path[0] == L"employees")
	{
		if (path.size() == 1)
		{
			request.reply(web::http::status_codes::BadRequest, L"No ID");
		}
		else if (path.size() > 1 && isNumber(path[1]))
		{
			try
			{
				utility::stringstream_t queryStr;
				queryStr << L"{\"statements\":[{\"statement\":\"MATCH (e:Employee) where e.id=";
				queryStr << L"\\\"";
				queryStr << path[1];
				queryStr << L"\\\"";
				queryStr << L" DETACH DELETE e \"}]}";

				web::http::http_request requestToNEO4j(web::http::methods::POST);

				requestToNEO4j.headers().add(L"Authorization", authStr);
				requestToNEO4j.headers().add(L"Content-Type", L"application/json");

				requestToNEO4j.set_body(web::json::value::parse(queryStr.str()));

				client->request(requestToNEO4j).then([&](const web::http::http_response& responseFromNEO4j)
					{
						request.reply(responseFromNEO4j.status_code(), responseFromNEO4j.extract_json().get());
					});
			}
			catch (std::exception& e)
			{
				request.reply(web::http::status_codes::Conflict, e.what());
			}
		}
	}
}

void handlePUT(web::http::http_request request)
{
	std::vector<utility::string_t>path = web::http::uri::split_path(web::http::uri::decode(request.relative_uri().path()));

	web::json::value answer = web::json::value::object();

	if(path.empty())
	{
		request.reply(web::http::status_codes::BadRequest, L"Bad request");
	}
	else if (path[0] == L"employees")
	{
		if (path.size() == 1)
		{  
			request.reply(web::http::status_codes::BadRequest, L"No ID");
		}
		else if (path.size() > 1 && isNumber(path[1]))
		{
			try
			{
				web::json::value json = request.extract_json().get();

				utility::stringstream_t queryStr;
				queryStr << L"{\"statements\":[{\"statement\":\"MATCH (e:Employee) where e.id=";
				queryStr << L"\\\"";
				queryStr << path[1];
				queryStr << L"\\\"";

				for(const auto& i : json.as_object())
				{
					queryStr << L" SET e.";
					queryStr << i.first;
					queryStr << L"=\\\"";
					queryStr << i.second.as_string();
					queryStr << L"\\\"";
				}
				queryStr << L"\\\" return e \"}]}";

				web::http::http_request requestToNEO4j(web::http::methods::POST);

				requestToNEO4j.headers().add(L"Authorization", authStr);
				requestToNEO4j.headers().add(L"Content-Type", L"application/json");

				requestToNEO4j.set_body(web::json::value::parse(queryStr.str()));

				client->request(requestToNEO4j).then([&](const web::http::http_response& responseFromNEO4j)
					{
						request.reply(responseFromNEO4j.status_code(),responseFromNEO4j.extract_json().get());
					});
			}
			catch (std::exception& e)
			{
				request.reply(web::http::status_codes::Conflict, e.what());
			}

		}
	}
}

void handlePOST(web::http::http_request request)
{
	std::vector<utility::string_t>path = web::http::uri::split_path(web::http::uri::decode(request.relative_uri().path()));

	web::json::value answer = web::json::value::object();

	if (path.empty())
	{

	}
	else if (path[0] == L"employees" && request.headers().content_type()== L"application/json")
	{
		if (path.size() == 1)
		{
			try {
				web::json::object body = request.extract_json().get().as_object();
				if (body[L"name"].as_string() != L"null" &&
					body[L"department"].as_string() != L"null")
				{
					web::http::http_request requestToNEO4j(web::http::methods::POST);

					requestToNEO4j.headers().add(L"Authorization", authStr);
					requestToNEO4j.headers().add(L"Content-Type", L"application/json");

					utility::stringstream_t queryStr;
					queryStr << L"{\"statements\":[{\"statement\":\"MATCH (e:Employee) where e.name=";
					queryStr << L"\\\"";
					queryStr << body[L"name"].as_string();
					queryStr << L"\\\" return e \"}]}";

					requestToNEO4j.set_body(web::json::value::parse(queryStr.str()));

					client->request(requestToNEO4j).then([&](web::http::http_response responseFromNEO4j)
						{
							try
							{
								std::cout << responseFromNEO4j.status_code() << std::endl;
								web::json::value json = responseFromNEO4j.extract_json().get();

								if (json[L"results"].as_array().at(0).as_object()[L"data"].as_array().size() != 0)
								{
									request.reply(web::http::status_codes::BadRequest, L"Employee already exists");
								}
								else
								{
									utility::stringstream_t queryStr;
									queryStr << L"{\"statements\":[{\"statement\":\"CREATE (e:Employee {name:\\\"";
									queryStr << body[L"name"].as_string();
									queryStr << L"\\\", department:\\\"";
									queryStr << body[L"department"].as_string();
									queryStr << L"\\\"}) return e \"}]}";

									requestToNEO4j.set_body(web::json::value::parse(queryStr.str()));

									client->request(requestToNEO4j).then([&](web::http::http_response responseFromNEO4j)
										{
											try
											{
												std::cout << responseFromNEO4j.status_code() << std::endl;
												web::json::value json = responseFromNEO4j.extract_json().get();

												request.reply(web::http::status_codes::OK, json);
											}
											catch (std::exception& e)
											{
												std::cout << e.what() << std::endl;
											}
										}).wait();
								}	
							}
							catch (std::exception& e)
							{
								std::cout << e.what() << std::endl;
							}
						}).wait();
				}
			}
			catch (std::exception& e)
			{
				request.reply(web::http::status_codes::BadRequest, e.what());
			}
		}
	}
}

void handleGET(web::http::http_request request)
{
	std::vector<utility::string_t>path = web::http::uri::split_path(web::http::uri::decode(request.relative_uri().path()));
	std::map<utility::string_t, utility::string_t> queries = web::http::uri::split_query(web::http::uri::decode(request.relative_uri().query()));
	//web::json::json

	web::json::value answer = web::json::value::object();

	size_t pathSize = path.size();

	if (path.empty())
	{
		request.reply(web::http::status_codes::BadRequest, L"Empty path");
	}
	else if (path[0] == L"employees")						
	{
		try
		{
			if (path.size() == 1)				//				/employees GET
			{
				web::json::object* filterQuery = nullptr;
				web::json::object* sortQuery = nullptr;

				for (auto const& query : queries)			// iteracja po query
				{
					if (query.first == L"filter")								// je¿eli jest filter, koniecznie obiekt
					{
						filterQuery = new web::json::object(web::json::value::parse(query.second).as_object());
					}
					else if (query.first == L"sort")							// je¿eli jest sort, koniecznie obiekt
					{
						sortQuery = new web::json::object(web::json::value::parse(query.second).as_object());
					}
				}

				utility::stringstream_t queryStr; 

				queryStr << L"{\"statements\":[{\"statement\":\"";
				queryStr << L"MATCH(e:Employee) ";
				
				for (web::json::object::iterator i = filterQuery->begin(); i != filterQuery->end(); i++)
				{
					queryStr << L"WITH * WHERE e." << i->first << L"=\\\"" << i->second.as_string() << L"\\\" ";
				}
				queryStr << L"return e";
				for (web::json::object::iterator i = sortQuery->begin(); i != sortQuery->end(); i++)
				{
					queryStr << L"ORDER BY e." << i->first << L" " << (i->second.as_integer() ? L"ASC" : L"DESC") << L" ";
				}
				queryStr << L"\"}]}";

				std::wcout << queryStr.str() << std::endl;

				web::http::http_request requestToNEO4j(web::http::methods::POST);

				requestToNEO4j.headers().add(L"Authorization", authStr);
				requestToNEO4j.headers().add(L"Content-Type", L"application/json");
				requestToNEO4j.set_body(web::json::value::parse(queryStr.str()));
				
				client->request(requestToNEO4j).then([&](web::http::http_response responseFromNEO4j)
					{
						try
						{
							std::cout << responseFromNEO4j.status_code() << std::endl;
							responseFromNEO4j.extract_json().then([&](web::json::value json)
								{
									request.reply(web::http::status_codes::OK, json);
								}).wait();
						}
						catch (std::exception& e)
						{
							std::cout << e.what() << std::endl;
						}
					}).wait();
				delete filterQuery;
				delete sortQuery;
			}
			else if (path.size() > 1 && isNumber(path[1]))					//
			{

			}
		}
		catch (std::exception& e)
		{
			request.reply(web::http::status_codes::BadRequest, e.what());
			std::wcout << e.what() << std::endl;
		}
	}
}