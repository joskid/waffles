/*
  The contents of this file are dedicated by all of its authors, including

    Michael S. Gashler,
    anonymous contributors,

  to the public domain (http://creativecommons.org/publicdomain/zero/1.0/).

  Note that some moral obligations still exist in the absence of legal ones.
  For example, it would still be dishonest to deliberately misrepresent the
  origin of a work. Although we impose no legal requirements to obtain a
  license, it is beseeming for those who build on the works of others to
  give back useful improvements, or pay it forward in their own field. If
  you would like to cite us, a published paper about Waffles can be found
  at http://jmlr.org/papers/volume12/gashler11a/gashler11a.pdf. If you find
  our code to be useful, the Waffles team would love to hear how you use it.
*/

#ifndef RECOMMENDER
#define RECOMMENDER


#include <GClasses/GBitTable.h>
#include <GClasses/GDynamicPage.h>
#include <string>
#include <vector>
#include <map>


namespace GClasses
{
	class GRand;
	class GDom;
	class GDomNode;
}
class Ratings;
class Item;
class Account;
class Server;
using namespace GClasses;


#define PERSONALITY_DIMS 5 // (one of these is used for the bias)
#define ON_RATE_TRAINING_ITERS 50000
#define ON_TRAIN_TRAINING_ITERS 1000000
#define ON_STARTUP_TRAINING_ITERS 250000
#define LEARNING_RATE 0.001
#define REGULARIZATION_TERM 0.00001



class User
{
protected:
	std::string m_username;
	std::vector<Ratings*> m_ratings; // This is the training data for learning the user's personality vector.
	std::vector<double> m_personality; // This vector represents the user with respect to our model. That is, given the user's personality vector, our model should be able to predict the ratings of this user with some accuracy.

public:
	User(std::string username, GRand& rand);
	~User();

	std::vector<Ratings*>& ratings() { return m_ratings; }
	std::vector<double>& personality() { return m_personality; }

	static User* fromDom(GDomNode* pNode, GRand& rand);

	GDomNode* toDom(GDom* pDoc);

	const char* username() { return m_username.c_str(); }
	void addRating(size_t topic, size_t itemId, float rating);
	void updateRating(size_t topic, size_t itemId, float rating);
	void withdrawRating(size_t topic, size_t itemId);
	void swapItems(size_t topic, size_t a, size_t b);
	float predictRating(Item& item);
	bool getRating(size_t topic, size_t itemId, float* pOutRating);

};


class Item
{
protected:
	std::string m_left;
	std::string m_right;
	std::string m_submitter;
	time_t m_date; // the date this item was submitted
	std::vector<double> m_weights; // used to predict the rating from a user's personality vector

public:
	Item(const char* szLeft, const char* szRight, const char* szSubmitter, time_t date, GRand* pRand);
	Item(GDomNode* pNode, GRand* pRand);

	const char* left() { return m_left.c_str(); }
	const char* right() { return m_right.c_str(); }
	std::vector<double>& weights() { return m_weights; }

	GDomNode* toDom(GDom* pDoc);

	double predictRating(const std::vector<double>& personality) const;

	// This method adjusts the weights in the opposite direction of the gradient of
	// the squared-error with respect to the weights.
	void trainWeights(double target, double learningRate, const std::vector<double>& personality);

	// This method adjusts the personality vector in the opposite direction of the gradient of
	// the squared-error with respect to the personality vector.
	void trainPersonality(double target, double learningRate, std::vector<double>& personality) const;
};




// A grouping of items
class Topic
{
protected:
	std::string m_descr;
	std::vector<Item*> m_items;

public:
	Topic(const char* szDescr);

	~Topic();

	GDomNode* toDom(GDom* pDoc);
	static Topic* fromDom(GDomNode* pNode, GRand* pRand);
	void flushItems();
	size_t size() { return m_items.size(); }
	Item& item(size_t id);
	const char* descr() { return m_descr.c_str(); }
	void addItem(const char* szLeft, const char* szRight, const char* szUsername, time_t date, GRand* pRand);
	void deleteItemAndSwapInLast(size_t itemId);
};




/*
class Rating
{
protected:
	User* m_pUser;
	Item* m_pItem;
	float m_rating;

public:
	Rating(User* pUser, Item* pItem, float rating)
	: m_pUser(pUser), m_pItem(pItem), m_rating(rating)
	{
	}

	Rating(const Rating& copyme)
	{
		(*this) = copyme;
	}

	Rating& operator=(const Rating& copyme)
	{
		m_pUser = copyme.m_pUser;
		m_pItem = copyme.m_pItem;
		m_rating = copyme.m_rating;
		return *this;
	}

	~Rating()
	{
	}
};
*/





class Ratings
{
public:
	std::map<size_t, float> m_map;
	std::vector<std::pair<size_t, float> > m_vec;

	void addRating(size_t itemId, float rating);
	void updateRating(size_t itemId, float rating);
	void withdrawRating(size_t itemId);
	void swapItems(size_t a, size_t b);
};







/// Collects users, items, and ratings
class Recommender
{
protected:
	GRand& m_rand;
	std::vector<User*> m_users;
	std::map<std::string,User*> m_userMap;
	std::vector<Topic*> m_topics;

public:
	Recommender(GRand& rand);
	~Recommender();

	void flush();
	GDomNode* serialize(GDom* pDoc);
	void deserialize(const GDomNode* pNode);
	const std::vector<Topic*>& topics() { return m_topics; }
	const std::vector<User*>& users() { return m_users; }
	User* findUser(const char* szUsername);
	User* findOrMakeUser(const char* szUsername);

	// Takes ownership of pUser
	void addUser(User* pUser);

	void addItem(size_t topic, const char* szLeft, const char* szRight, const char* szUsername);
	void proposeTopic(const char* username, const char* szDescr);
	void newTopic(const char* szDescr);
	void refineModel(size_t topic, size_t iters); // trains both personalities and weights
	void refinePersonality(User* pUser, size_t topic, size_t iters); // trains just the personalities
};


/// Makes pages that collect survey responses and visualize the results
class Survey
{
public:
	/// Generates a page of slider bars for users to take a survey
	static void pageSurvey(Server* pServer, GDynamicPageSession* pSession, std::ostream& response);

	/// Generates a page that lets users submit new survey questions
	static void pageNewSurveyItem(Server* pServer, GDynamicPageSession* pSession, std::ostream& response);

	/// Generates a page that summarizes survey responses
	static void pageStats(Server* pServer, GDynamicPageSession* pSession, std::ostream& response);

	/// Generates an SVG plot of users
	static void plotUsers(Server* pServer, GDynamicPageSession* pSession, std::ostream& response);

	/// Generates an SVG plot of items
	static void plotItems(Server* pServer, GDynamicPageSession* pSession, std::ostream& response);

	/// A page that lets users update their survey responses
	static void pageUpdateResponses(Server* pServer, GDynamicPageSession* pSession, std::ostream& response);

protected:
	static void makeSliderScript(std::ostream& response);
	static void makeUrlSlider(Server* pServer, Account* pAccount, size_t itemId, std::ostream& response);
	static double computeVariance(double* pCentroid, Topic& topic, size_t topicId, User** pUsers, size_t accCount);
	static size_t divideAccounts(Topic& topic, size_t topicId, User** pUsers, size_t accCount, size_t itm);
	static void makeTree(Server* pServer, Topic& topic, size_t topicId, GBitTable& bt, User** pUsers, size_t accCount, std::ostream& response, std::vector<char>& prefix, int type);
	static void makeItemBody(GDynamicPageSession* pSession, std::ostream& response, size_t topicId, size_t itemId, Item& item, User** pUsers, size_t accCount);
	static void makeUserBody(GDynamicPageSession* pSession, std::ostream& response, User* pA, User* pB, size_t topicId, Topic& topic);
};



#endif // RECOMMENDER
