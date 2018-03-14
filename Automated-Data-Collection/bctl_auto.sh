#!/bin/bash
i=0
MAC=("123" "456")
cat bctl.txt | 
{
# we get the MAC addresses from bctl.txt
while read LINE
do
  MAC[$i]="$LINE"
  i=$i+1
done

# we execute bctl_helper and set up motion_data.txt
rm motion_data.txt
touch motion_data.txt
#we pass the MAC addresses into the except script
exp='./bctl_helper.exp '
eval $exp${MAC[1]}${MAC[2]}
}
