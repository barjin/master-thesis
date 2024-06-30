# Social network analysis in academic environment

This repository contains the TeX markup files for the master thesis of Jindřich Bär, regarding social network analysis in academic environment.

As of the time of writing this README, the thesis is very much WIP. 

Not a fan of reading raw TeX code? [Read this work as a PDF](https://jindrich.bar/master-thesis/bar-social-network-analysis-in-academic-environment-2024.pdf) instead


## How is it going?

In the box below, you can see the thesis assignment and the current status of the individual parts of the thesis.

> 🔴 Student při řešení navrhne efektivní způsob transformace relačních dat o entitách na UK na grafový model. 
> - This part is not done yet. The data has been collected and processed, but the actual transformation into a graph model has been only done using the CSV Memgraph importer tool.

> 🔴 Dále student navrhne postup pro analýzu různých aspektů vzniklé sociální sítě (např. hledání komunit, výpočet centrality vrcholů atd.) a jejich význam a specifika pro aplikaci v akademickém prostředí.
> Výsledky student následně porovná s existujícími algoritmy pro analýzu sociálních sítí. 
> - Not done yet. This could be connected to the search reranking part. There, we are working with a real-time application that could benefit from a smart fast algorithm for (local?) SNA.

> 🟡 Student také zhodnotí využití sociální sítě pro zlepšení výkonu fulltextového vyhledávání (reranking výsledků) v aplikaci.
> - This part is mostly done - the chapter 5 (Social network boosted search ranking) talks about benchmarking the search ranking with and without the social network data.
>    - It is still missing the actual benchmark using the social network measures used for the reranking - but all the data for this is collected and ready to be processed.

> ✅ Součástí práce je experimentální implementace nástroje pro vizualizaci a prohlížení vzniklé sociální sítě do aplikace Charles Explorer (https://explorer.cuni.cz). 
>   - This part is fully done, the tool is implemented and is available at https://explorer.cuni.cz
>   - The details about the implementation are in the Chapter 6 (Visualising networks on the Web).