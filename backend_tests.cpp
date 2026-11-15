#define GWB_TESTING
#include "backend.cpp"
#include <cassert>

void testUrlAndFormUtilities() {
    assert(getDomain("https://www.example.com/path?q=1") == "example.com");
    assert(getDomain("http://sub.example.com:8080/page") == "sub.example.com");
    assert(getDomain("https://user:pass@casino.example./home") == "casino.example");
    assert(urlDecode("place+bet%21") == "place bet!");

    map<string, string> values = parseForm("url=https%3A%2F%2Fexample.com&score=75");
    assert(values["url"] == "https://example.com");
    assert(values["score"] == "75");
    assert(getNumber(values, "score") == 75);
    assert(getNumber(values, "missing") == 0);
}

void testTrieAndHashTable() {
    Trie trie;
    trie.insert("casino");
    trie.insert("betting");
    assert(trie.search("casino"));
    assert(trie.search("betting"));
    assert(!trie.search("casinos"));
    assert(!trie.search("bet"));

    DomainHashTable domains;
    domains.insert("known.example");
    domains.insert("casino.test");
    domains.insert("known.example"); // Duplicate insertion is ignored.
    assert(domains.search("known.example"));
    assert(domains.containsDomainOrParent("play.known.example"));
    assert(domains.containsDomainOrParent("known.example"));
    assert(!domains.containsDomainOrParent("unknown.example"));
}

void testStackAndQueue() {
    Queue<int> queue;
    int value = 0;
    assert(!queue.dequeue(value));
    queue.enqueue(10);
    queue.enqueue(20);
    queue.enqueue(30);
    assert(queue.size() == 3);
    assert(queue.dequeue(value) && value == 10);
    assert(queue.dequeue(value) && value == 20);
    assert(queue.dequeue(value) && value == 30);
    assert(queue.isEmpty());

    Stack<int> stack(2);
    stack.push(1);
    stack.push(2);
    stack.push(3); // Oldest item is discarded at capacity.
    assert(stack.size() == 2);
    assert(stack.top() == 3);
    stack.pop();
    assert(stack.top() == 2);
    stack.pop();
    assert(stack.isEmpty());
}

void testCppPageFeatureExtraction() {
    Evidence evidence;
    evidence.pageText =
        "Place bet now. Odds 1.85 and 2.10. Deposit funds. "
        "Casino roulette live betting sportsbook.";
    evidence.clickableLabels = "Place bet now | 1.85 | 2.10";
    evidence.inputHints = "stake amount";
    evidence.marketHints = "market market market";
    evidence.frameHints = "casino live-game";
    evidence.formHints = "UPI payment method deposit";

    extractPageEvidence(evidence);

    assert(evidence.oddsPatterns >= 2);
    assert(evidence.betActions >= 1);
    assert(evidence.stakeInputs >= 1);
    assert(evidence.marketGroups >= 3);
    assert(evidence.depositActions >= 1);
    assert(evidence.casinoEmbeds >= 1);
    assert(evidence.paymentForms >= 1);
    assert(evidence.gamblingTerms >= 2);
    assert(evidence.bettingControls >= 1);
    assert(evidence.clickableOdds >= 2);
    assert(evidence.walletSignals >= 1);
    assert(evidence.casinoTerms >= 1);
}

void testRiskClassification() {
    RiskEngine engine;

    Evidence highRisk;
    highRisk.pageText =
        "Place bet now. Odds 1.85 2.10 3.20. Deposit funds. "
        "Casino roulette blackjack live betting sportsbook poker.";
    highRisk.clickableLabels = "Place bet now | 1.85 | 2.10 | 3.20";
    highRisk.inputHints = "stake amount";
    highRisk.marketHints = "market market market market";
    highRisk.frameHints = "casino live-game";
    highRisk.formHints = "UPI payment method deposit";
    extractPageEvidence(highRisk);

    AnalysisResult high = engine.analyze(highRisk);
    assert(high.score >= 60);
    assert(high.status == "HIGH RISK - LIKELY GAMBLING");
    assert(!high.reasons.empty());

    Evidence clean;
    clean.pageText = "Welcome to a public library. Read articles and research papers.";
    extractPageEvidence(clean);
    AnalysisResult low = engine.analyze(clean);
    assert(low.score < 30);
    assert(low.status == "LOW EVIDENCE - NOT CONFIRMED");

    Evidence suspicious;
    suspicious.pageText = "Sportsbook odds and betting information.";
    extractPageEvidence(suspicious);
    AnalysisResult review = engine.analyze(suspicious);
    assert(review.score >= 0 && review.score <= 100);
}

void testDomainRuleAndJson() {
    RiskEngine engine;
    Evidence known;
    known.url = "https://jalwaclub3.com/play";
    AnalysisResult result = engine.analyze(known);
    assert(result.score >= 60);

    AnalysisResult sample;
    sample.score = 42;
    sample.status = "Needs review";
    sample.reasons.push_back("A \"quoted\" reason");
    string json = makeJson(sample);
    assert(json.find("\"score\":42") != string::npos);
    assert(json.find("quoted") != string::npos);
}


void testPatternCounterLimitsAndCaseHandling() {
    const string mixed = "CASINO casino Casino betting BETTING";
    assert(countPattern(mixed, R"(casino)") == 3);
    assert(countPattern(mixed, R"(betting)") == 2);
    assert(countPattern(mixed, R"(casino)", 2) == 2);
    assert(countPattern("", R"(casino)") == 0);
    assert(countPattern("ordinary text", R"(casino)") == 0);
}

void testEvidenceExtractionCaps() {
    Evidence evidence;
    for (int i = 0; i < 150; ++i) {
        evidence.pageText += " casino place bet 1.85 deposit ";
        evidence.clickableLabels += " place bet 1.85 ";
        evidence.inputHints += " stake ";
        evidence.marketHints += " market ";
        evidence.frameHints += " casino ";
        evidence.formHints += " deposit ";
    }

    extractPageEvidence(evidence);
    assert(evidence.oddsPatterns <= 100);
    assert(evidence.betActions <= 50);
    assert(evidence.stakeInputs <= 50);
    assert(evidence.marketGroups <= 100);
    assert(evidence.depositActions <= 50);
    assert(evidence.casinoEmbeds <= 20);
    assert(evidence.paymentForms <= 20);
    assert(evidence.gamblingTerms <= 100);
    assert(evidence.bettingControls <= 50);
    assert(evidence.clickableOdds <= 100);
}

void testDomainKeywordSignals() {
    RiskEngine engine;
    Evidence evidence;
    evidence.url = "https://casino.play.example/home";
    AnalysisResult result = engine.analyze(evidence);
    assert(result.score >= 15);

    bool foundKeywordReason = false;
    for (const string &reason : result.reasons) {
        if (reason.find("domain keyword") != string::npos) {
            foundKeywordReason = true;
        }
    }
    assert(foundKeywordReason);
}

void testRiskScoreUpperBound() {
    RiskEngine engine;
    Evidence evidence;
    evidence.url = "https://casino.example";
    evidence.pageText =
        "casino casino casino roulette blackjack baccarat slots jackpot "
        "place bet bet now add to bet slip confirm bet deposit withdraw "
        "sportsbook betting wager odds poker lottery live casino";
    evidence.clickableLabels =
        "place bet bet now add to bet slip 1.85 2.10 3.25";
    evidence.inputHints = "stake wager bet amount";
    evidence.marketHints = "market market market market market";
    evidence.frameHints = "casino slot roulette live-game";
    evidence.formHints = "card number payment method UPI deposit";
    extractPageEvidence(evidence);

    AnalysisResult result = engine.analyze(evidence);
    assert(result.score == 100);
    assert(result.score <= 100);
}

void testOrdinaryPageDoesNotTriggerGamblingRules() {
    Evidence evidence;
    evidence.pageText =
        "University library opening hours, research papers, student portal, "
        "course timetable, examination results and campus news.";
    evidence.clickableLabels = "Home | About us | Contact | Student portal";
    evidence.inputHints = "Search articles";
    evidence.marketHints = "";
    evidence.frameHints = "";
    evidence.formHints = "Search this website";
    extractPageEvidence(evidence);

    assert(evidence.oddsPatterns == 0);
    assert(evidence.betActions == 0);
    assert(evidence.stakeInputs == 0);
    assert(evidence.depositActions == 0);
    assert(evidence.casinoEmbeds == 0);
    assert(evidence.paymentForms == 0);
    assert(evidence.gamblingTerms == 0);
    assert(evidence.casinoTerms == 0);
}

void testUrlNormalizationEdgeCases() {
    assert(getDomain("WWW.Example.COM.") == "example.com");
    assert(getDomain("https://user@example.org/path") == "example.org");
    assert(getDomain("http://localhost:8765/analyze") == "localhost");
    assert(getDomain("https://a.b.casino.example/path") == "a.b.casino.example");
    assert(getDomain("").empty());
}

void testJsonEscapingAndEmptyReasons() {
    AnalysisResult empty;
    empty.score = 0;
    empty.status = "LOW EVIDENCE";
    string emptyJson = makeJson(empty);
    assert(emptyJson.find("\"reasons\":[]") != string::npos);

    AnalysisResult quoted;
    quoted.score = 55;
    quoted.status = "Review\\Required";
    quoted.reasons.push_back("Line one\nLine two");
    string escaped = makeJson(quoted);
    assert(escaped.find("\\\\Required") != string::npos);
    assert(escaped.find("Line one\\nLine two") != string::npos);
}

void testFormParserIgnoresInvalidParts() {
    map<string, string> parsed = parseForm("broken&score=12&empty=&url=https%3A%2F%2Fsite.test");
    assert(parsed.size() == 3);
    assert(parsed["score"] == "12");
    assert(parsed["empty"].empty());
    assert(parsed["url"] == "https://site.test");
}

void testIndependentRiskEvidenceReasons() {
    Evidence evidence;
    evidence.pageText = "Place bet now. Stake amount. Deposit funds. Casino slots.";
    evidence.clickableLabels = "Place bet now";
    evidence.inputHints = "stake amount";
    evidence.frameHints = "casino slots";
    extractPageEvidence(evidence);

    AnalysisResult result = RiskEngine().analyze(evidence);
    assert(result.score > 0);
    assert(!result.reasons.empty());

    bool hasBetReason = false;
    bool hasStakeReason = false;
    for (const string &reason : result.reasons) {
        if (reason.find("Betting actions") != string::npos) hasBetReason = true;
        if (reason.find("Stake input") != string::npos) hasStakeReason = true;
    }
    assert(hasBetReason);
    assert(hasStakeReason);
}

int main() {
    testUrlAndFormUtilities();
    testTrieAndHashTable();
    testStackAndQueue();
    testCppPageFeatureExtraction();
    testRiskClassification();
    testDomainRuleAndJson();
    testPatternCounterLimitsAndCaseHandling();
    testEvidenceExtractionCaps();
    testDomainKeywordSignals();
    testRiskScoreUpperBound();
    testOrdinaryPageDoesNotTriggerGamblingRules();
    testUrlNormalizationEdgeCases();
    testJsonEscapingAndEmptyReasons();
    testFormParserIgnoresInvalidParts();
    testIndependentRiskEvidenceReasons();

    cout << "All Gambling Website Blocker C++ tests passed.\n";
    return 0;
}
