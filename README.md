# InterleavingPackerUnpacker
Created for editing game files of Turok2 (1998 one) but it can be used for other purposes.

This program finds a hierarchy/structures in .lsm and .lss files and extracts them to files with unknown extension but I am not sure if the decryption of the Turok2 files is done correctly. I don't know much about reverse engineering so we only managed to swap the textures and models among themselves, it can result in fun effects :D
I also managed to change position, size, rotation and even models of some parts of levels eg. chairs on the Harbour5 level:
![alt Changed chair on H5 ss1](https://cdn.discordapp.com/attachments/272226370158067713/833374543631745054/unknown.png)
![alt Changed chair on H5 ss2](https://cdn.discordapp.com/attachments/272226370158067713/833374613744386058/unknown.png)

Check out the screenshots folder to learn more.

Structure of the Turok2 .lsm files:
`
sb0 -> models
sb1 -> actor attributes?
sb2 -> actor types?
sb3 -> textures?
sb4 -> particles!
sb5,sb6,sb7 -> levels! (sound shaders? event table?)
sb8 -> warp IDs? / spawn-points!
sb9,sb10 -> ? (empty in .lsm and .lss)
sb11 -> weapon effects (same in every .lsm, smaller in .lss)
sb12 -> ? (same in .lsm, 16bytes longer in .lss)
sb13 -> ? (same in .lsm, 24bytes longer in .lss)
sb14 -> ? (same in .lsm, 8bytes longer in .lss)
sb15 -> ? (same in .lsm, 8bytes longer in .lss)
sb16 -> gui textures! 
sb17 -> defines sth for gui textures (ids? offsets?)
sb18 -> ? (lector sounds/issues? cos big in ctf and arena and in .lss, but small 

in rok?)
sb19 -> ?
sb20 -> ? (empty in .lsm and .lss)
 
rok->5->17 = graveryard
rok->5->17->6 = fog on the floor/ground of graveryard
rok->5->17->3 = positions of pickables and some walls

rok->5->0->3 = picables, gates and bridges, barrels (generally positions of models?)
rok->5->0->1 = ground hitboxes?

rok->5->9 = Harbour5 
rok->5->9->5->9->1 = small ladder
rok->5->9->5->14->1 = House with the rocket launcher (scorpion) on the roof
rok->5->9->5->15->3 = ? maybe this single box near plasma ?
rok->5->9->5->20->3 = ?
rok->5->9->5->21->3 = ?
rok->5->9->5->22->1 = area of second plasma in the small tunnel near boxes
rok->5->9->5->27->1 = left side of the fountain

rok->5->9->5->14->1 // House with the rocket launcher (scorpion) on the roof
	1 segment: the right-down chair (amount of segments and its length is in first 2 words = 8 bytes)
		1 word: ?? (1 word = 4 bytes)
		2,3,4 word: size (float)  
		5 word: x position (float)
		6 word: y position (float)
		7 word: z position (float)
		8 word: ?
		9 word: ?
		10 word: ?
		11 word: first 2 bytes determine displayed model (EA 00 B3 01 = closed book)
		12 word: ?
		13 word: y-axis rotation
	2 segment: right-top chair
  `
