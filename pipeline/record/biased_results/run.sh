for ((i = 1; i < 40; i ++))
do
	echo $i
	rm agent_dse.model
	rm target_dse.model
	./train -s 6 -e 0.0001 agent_dse
	./train -s 6 -e 0.0001 target_dse
	./predict agent_dse agent_dse.model agent_results/agent_dse$i.out
	./predict target_dse target_dse.model target_results/target_dse$i.out 
	./drnt 0 > results/res$i.out
done


