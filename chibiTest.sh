#! /bin/bash
CYAN='\033[0;36m'
GREEN='\033[0;32m'
RED='\033[0;31m'
GREY='\033[1;30m'
NC='\033[0m'


curlArgs=(-s -w "\n%{http_code}")
count=0
passCount=0
chibiServerPID=0

testRoute() {
	count=$((count + 1))
	status_pass=0
	body_pass=0
	status_code=0
	body=""
	response_data=""

	echo -en ">> test ${CYAN}$1 ${NC}($2, $3) > "
	
	response_data=$(curl "${curlArgs[@]}" http://localhost:5000$3)
	response=(${response_data[@]}) # convert to array
	
	if [ ${#response[@]} -gt 1 ]; then
		status_code=${response[${#response[@]}-1]} # get last element (last line)
	else 
		status_code=${response[0]};
	fi
	body=${response[@]::${#response[@]}-1} # get all elements except last

	# check status from curl is expected
	if test $2 = $status_code; then
		echo -e "${GREEN}status PASS${NC}";
		status_pass=1
	else
		echo -e "${RED}status FAIL${NC} ${GREY}($2 != $status_code)${NC}";
	fi
	
	# check and test body
	if [ $# -eq 4 ]; then
		if [ $4 = $body ]; then
			body_pass=1;
		else
			echo -e "${RED} body FAIL ${GREY}($4 != $body)${NC}";
		fi
	else
		# not tested so pass
		body_pass=1;
	fi

	# update global pass count
	if [ $status_pass -eq 1 ] && [ $body_pass -eq 1 ]; then
		passCount=$((passCount + 1));
	fi
}
echo "####################"
echo "# chibi test suite #"
echo "####################"
echo

# set up
echo "> starting chibiServer"
if [ $(ps aux | grep ./chibiServer | wc -l) -gt 1 ]; 
	then echo "> another chibiServer detected, tests may fail";
fi
./chibiServer > /dev/null 2>&1 &
chibiServerPID=$!
sleep 1
# tests
echo -en "\nTests:\n"

# testRoute (description, status_code, route, body)
testRoute "hello route is OK" "200" "/hello" "HELLO"
testRoute "non-existent route is not found" "404" "/ping"
testRoute "static index route is OK" "200" "/"  $(cat static/index.html)


# tear down
if [ $chibiServerPID -gt 0 ]; then
 	kill -INT $chibiServerPID; # tell chibi to shutdown gracefully
	echo -e "\n> tearing down chibiServer";
fi


# results
echo -e "\nResults:"
echo "-------"
echo "Total -  $count"
echo -e "${GREEN}Passed - $passCount${NC}"

# check for failed tests and exit appropriately
failed=$((count - passCount))
if [ $failed -gt 0 ]; then
 echo -e "${RED}Failed - $failed ${NC}";
 exit 1;
fi

exit
